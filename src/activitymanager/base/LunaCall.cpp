// Copyright (c) 2009-2021 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include <stdexcept>
#include <luna/MojLunaMessage.h>

#include "base/LunaCall.h"
#include "util/Logging.h"
#include "service/BusConnection.h"

const int LunaCall::kMaxRetry = 5;
const MojUInt32 LunaCall::kUnlimited = MojServiceRequest::Unlimited;
const std::string LunaCall::kServiceStatusReady = "ready";
const std::string LunaCall::kServiceStatusRunning = "running";
const char* LunaCall::kBusErrorServiceBusy = "ServiceBusy";

unsigned LunaCall::s_serial = 0;

LunaCall::LunaCall(bool isClient, const LunaURL& url, const MojObject& params, MojUInt32 replies)
    : m_isClient(isClient)
    , m_url(url)
    , m_params(params)
    , m_replies(replies)
    , m_subSerial(0)
    , m_retryCount(0)
{
    m_serial = s_serial++;
}

LunaCall::~LunaCall()
{
    LOG_AM_DEBUG("[Call %u] Cleaning up", m_serial);

    cancel();
}

MojErr LunaCall::call()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    return call("","");
}

MojErr LunaCall::call(std::shared_ptr<Activity> activity)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    return call(activity->getCreator().getId(),activity->getRequesterExeName());
}

MojErr LunaCall::call(std::string proxyRequester, std::string requesterExeName)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);

    if (proxyRequester.empty()) {
        LOG_AM_DEBUG("[Call %u] Calling %s %s", m_serial,
                     m_url.getString().c_str(), MojoObjectJson(m_params).c_str());
    } else {
        LOG_AM_DEBUG("[Call %u] (Proxy for %s) Calling %s %s",
                     m_serial, proxyRequester.c_str(), m_url.getString().c_str(),
                     MojoObjectJson(m_params).c_str());
    }

    MojErr err;

    if (m_handler.get()) {
        m_handler->cancel();
        m_handler.reset();
    }

    MojRefCountedPtr<MojServiceRequest> req;

    MojLunaService* service = nullptr;
    if (m_isClient)
        service = &BusConnection::getInstance().getClient();
    else
        service = &BusConnection::getInstance().getService();

    if (proxyRequester.empty()) {
        err = service->createRequest(req);
        MojErrCheck(err);
    } else {
        LOG_AM_DEBUG("Proxy Requester service: %s ", proxyRequester.c_str());
        const MojChar* const busServiceName1 = "com.palm.bus";
        const MojChar* const busServiceName2 = "com.palm.luna.bus";
        const MojChar* const busServiceName3 = "com.webos.service.bus";
        MojString targetService;
        targetService.assign(m_url.getTargetService());
        if( targetService.compare(busServiceName1) == 0 ||
           targetService.compare(busServiceName2) == 0 ||
           targetService.compare(busServiceName3) == 0 )
        {
             LOG_AM_DEBUG("Use LSCall for target service: %s", targetService.data());
             err = service->createRequest(req, proxyRequester.c_str());
             MojErrCheck(err);
        }
        else
        {
             LOG_AM_DEBUG("Use LSCallProxy for target service: %s", targetService.data());
             err = service->createRequest(req, requesterExeName.c_str(),"",proxyRequester.c_str());
             MojErrCheck(err);
        }
    }

    MojRefCountedPtr<LunaCall::LunaCallMessageHandler> handler(
            new LunaCallMessageHandler(shared_from_this(), m_subSerial++));
    MojAllocCheck(handler.get());

    err = req->send(handler->getResponseSlot(), m_url.getTargetService(),
                    m_url.getMethod(), m_params, m_replies);
    MojErrCheck(err);

    /* Store handler for later cancel... */
    m_handler = handler;
    m_proxyRequester = proxyRequester;
    m_proxyRequesterExe = requesterExeName;

    return MojErrNone;
}

MojErr LunaCall::retry()
{
    if (++m_retryCount > kMaxRetry) {
        return MojErrInternal;
    }

    LOG_AM_INFO(MSGID_SERVICE_DOWN, 3,
                PMLOGKFV("serial", "%u", m_serial),
                PMLOGKS("service", m_url.getTargetService().data()),
                PMLOGKFV("retry", "%u", m_retryCount), "");

    usleep(100 * 1000);

    return call(m_proxyRequester,m_proxyRequesterExe);
}

MojErr LunaCall::callStatus()
{
    MojErr err;

    if (m_statusHandler.get()) {
        m_statusHandler->cancel();
        m_statusHandler.reset();
    }

    MojRefCountedPtr<MojServiceRequest> req;
    MojLunaService* service = &BusConnection::getInstance().getClient();
    err = service->createRequest(req);
    MojErrCheck(err);

    MojRefCountedPtr<LunaCall::LunaCallMessageHandler> handler(
            new LunaCallMessageHandler(shared_from_this(), m_subSerial++,
                                       &LunaCall::handleStatusResponseWrapper));
    MojAllocCheck(handler.get());

    MojObject params;
    err = params.put("serviceName", m_url.getTargetService());
    MojErrCheck(err);

    err = req->send(handler->getResponseSlot(), "com.webos.service.bus",
                    "signal/registerServiceStatus", params, kUnlimited);
    MojErrCheck(err);

    /* Store handler for later cancel... */
    m_statusHandler = handler;

    LOG_AM_INFO(MSGID_SERVICE_BUSY, 2,
                PMLOGKFV("serial", "%u", m_serial),
                PMLOGKS("service", m_url.getTargetService().data()), "");

    return MojErrNone;
}

void LunaCall::cancel()
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Call %u] Cancelling call %s", m_serial, m_url.getString().c_str());

    if (m_handler.get()) {
        m_handler->cancel();
        m_handler.reset();
    }
    if (m_statusHandler.get()) {
        m_statusHandler->cancel();
        m_statusHandler.reset();
    }
}

unsigned LunaCall::getSerial() const
{
    return m_serial;
}

/* Ensure Standardized logging and exception protection are in place */
void LunaCall::handleResponseWrapper(MojServiceMessage *msg, MojObject& response, MojErr err)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Call %u] %s: Received response %s",
                 m_serial, m_url.getString().c_str(), MojoObjectJson(response).c_str());

    /* XXX If response count reached, cancel call. */
    try {
        std::string error;
        if (isBusError(msg, response, error)) {
            MojErr err2 = MojErrLuna;
            if (error == kBusErrorServiceBusy) {
                if ((err2 = callStatus()) != MojErrNone) {
                    LOG_AM_WARNING(MSGID_SERVICE_BUSY, 3,
                                   PMLOGKFV("serial", "%u", m_serial),
                                   PMLOGKS("service", m_url.getTargetService().data()),
                                   PMLOGKFV("err", "%d", err2),
                                   "Failed to status call");
                }
            } else if (error == LUNABUS_ERROR_SERVICE_DOWN) {
                if ((err2 = retry()) != MojErrNone) {
                    LOG_AM_WARNING(MSGID_SERVICE_DOWN, 4,
                                   PMLOGKFV("serial", "%u", m_serial),
                                   PMLOGKS("service", m_url.getTargetService().data()),
                                   PMLOGKFV("err", "%d", err2),
                                   PMLOGKFV("retry", "%d", m_retryCount),
                                   "Failed to call retry");
                }
            }
            if (MojErrNone == err2) {
                return;
            }
        }
        handleResponse(msg, response, err);
    } catch (const std::exception& except) {
        LOG_AM_ERROR(MSGID_CALL_RESP_UNHANDLED_EXCEPTION, 3,
                     PMLOGKFV("serial", "%u", m_serial),
                     PMLOGKS("Url", m_url.getString().c_str()),
                     PMLOGKS("Exception", except.what()),
                     "Unhandled exception occurred processing response %s",
                     MojoObjectJson(response).c_str());
    } catch (...) {
        LOG_AM_ERROR(MSGID_CALL_RESP_UNKNOWN_EXCEPTION, 2,
                     PMLOGKFV("serial", "%u", m_serial),
                     PMLOGKS("Url", m_url.getString().c_str()),
                     "Unhandled exception of unknown type occurred processing response %s",
                     MojoObjectJson(response).c_str());
    }
}

void LunaCall::handleStatusResponseWrapper(MojServiceMessage *msg, MojObject& response, MojErr err)
{
    if (err != MojErrNone) {
        LOG_AM_WARNING(MSGID_SERVICE_BUSY, 0, "Failed status call for %s: %s",
                       m_url.getTargetService().data(), MojoObjectJson(response).c_str());
        m_statusHandler->cancel();
        return;
    }

    MojString status;
    bool found = false;
    MojErr err2 = response.get("status", status, found);
    if (err2 != MojErrNone || !found) {
        LOG_AM_WARNING(MSGID_SERVICE_BUSY, 0, "Failed status call for %s: %s",
                       m_url.getTargetService().data(), MojoObjectJson(response).c_str());
        m_statusHandler->cancel();
        return;
    }

    LOG_AM_INFO(MSGID_SERVICE_BUSY, 2,
                PMLOGKFV("serial", "%u", m_serial),
                PMLOGJSON("response", MojoObjectJson(response).c_str()), "");

    if (kServiceStatusReady != status.data() && kServiceStatusRunning != status.data()) {
        return;
    }

    call(m_proxyRequester,m_proxyRequesterExe);
    m_statusHandler->cancel();
}

bool LunaCall::isPermanentFailure(MojServiceMessage *msg, const MojObject& response, MojErr err)
{
    if (err == MojErrNone)
        return false;

    if (!isProtocolError(msg, response, err)) {
        return err;
    }

    MojLunaMessage *lunaMsg = dynamic_cast<MojLunaMessage *>(msg);
    if (!lunaMsg) {
        LOG_AM_ERROR(MSGID_NON_LUNA_BUS_MSG, 0,
                     "IsPermanentFailure() : Message %s did not originate from the Luna Bus",
                     MojoObjectJson(response).c_str());
        throw std::runtime_error("Message did not originate from the Luna Bus");
    }

    const MojChar *method = lunaMsg->method();
    if (!method)
        return false;
    else if (!strcmp(method, LUNABUS_ERROR_PERMISSION_DENIED))
        return true;
    else if (!strcmp(method, LUNABUS_ERROR_SERVICE_NOT_EXIST))
        return true;
    else if (!strcmp(method, LUNABUS_ERROR_UNKNOWN_METHOD))
        return true;

    return false;
}

bool LunaCall::isPermanentBusFailure(MojServiceMessage *msg, const MojObject& response, MojErr err)
{
    std::string error;
    if (isBusError(msg, response, error)) {
        if (LUNABUS_ERROR_PERMISSION_DENIED == error ||
                LUNABUS_ERROR_SERVICE_NOT_EXIST == error ||
                LUNABUS_ERROR_UNKNOWN_METHOD == error) {
            return true;
        }
    }
    return false;
}

bool LunaCall::isProtocolError(MojServiceMessage *msg, const MojObject& response, MojErr err)
{
    if (err == MojErrNone)
        return false;

    MojLunaMessage *lunaMsg = dynamic_cast<MojLunaMessage *>(msg);
    if (!lunaMsg) {
        LOG_AM_ERROR(MSGID_NON_LUNA_BUS_MSG, 0,
                     "IsProtocolError() : Message %s did not originate from the Luna Bus",
                     MojoObjectJson(response).c_str());
        throw std::runtime_error("Message did not originate from the Luna Bus");
    }

    const MojChar *category = lunaMsg->category();
    if (!category) {
        return false;
    } else {
        return (!strcmp(LUNABUS_ERROR_CATEGORY, lunaMsg->category()));
    }
}

bool LunaCall::isBusError(MojServiceMessage *msg, const MojObject& response, std::string& error)
{
    MojLunaMessage *lunaMsg = dynamic_cast<MojLunaMessage *>(msg);
    if (!lunaMsg) {
        LOG_AM_ERROR(MSGID_NON_LUNA_BUS_MSG, 0,
                     "%s() : Message %s did not originate from the Luna Bus",
                     __FUNCTION__, MojoObjectJson(response).c_str());
        throw std::runtime_error("Message did not originate from the Luna Bus");
    }

    const MojChar *category = lunaMsg->category();
    const MojChar *method = lunaMsg->method();
    if (category && !strcmp(LUNABUS_ERROR_CATEGORY, category) && method) {
        error = method;
        return true;
    }

    return false;
}

void LunaCall::handleResponse(MojObject& response, MojErr err)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    LOG_AM_DEBUG("[Call %u] %s: Unhandled response \"%s\"",
                 m_serial, m_url.getString().c_str(), MojoObjectJson(response).c_str());
}

/* Thunk to old-style message, if not overridden */
void LunaCall::handleResponse(MojServiceMessage *msg, MojObject& response, MojErr err)
{
    LOG_AM_TRACE("Entering function %s", __FUNCTION__);
    handleResponse(response, err);
}

LunaCall::LunaCallMessageHandler::LunaCallMessageHandler(
        std::shared_ptr<LunaCall> call, unsigned serial, Callback callback)
    : m_call(call)
    , m_responseSlot(this, &LunaCall::LunaCallMessageHandler::handleResponse)
    , m_serial(serial)
    , m_callback(callback)
{
}

LunaCall::LunaCallMessageHandler::~LunaCallMessageHandler()
{
}

MojServiceRequest::ExtendedReplySignal::Slot<LunaCall::LunaCallMessageHandler>&
LunaCall::LunaCallMessageHandler::getResponseSlot()
{
    return m_responseSlot;
}

void LunaCall::LunaCallMessageHandler::cancel()
{
    if (!m_call.expired()) {
        LOG_AM_DEBUG("[Call %u] [Handler %u] Cancelling", m_call.lock()->getSerial(), m_serial);
    } else {
        LOG_AM_DEBUG("[Handler %u] Call expired, Cancelling", m_serial);
    }

    m_responseSlot.cancel();
}

MojErr LunaCall::LunaCallMessageHandler::handleResponse(MojServiceMessage *msg, MojObject& response, MojErr err)
{
    if (m_call.expired()) {
        LOG_AM_DEBUG("[Handler %u] Response %s received for expired call",
                     m_serial, MojoObjectJson(response).c_str());
        m_responseSlot.cancel();
        return MojErrInvalidArg;
    }

    ((*m_call.lock().get()).*m_callback)(msg, response, err);

    return MojErrNone;
}
