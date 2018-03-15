// Copyright (c) 2009-2018 LG Electronics, Inc.
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

#ifndef __WHERE_MATCHER_H__
#define __WHERE_MATCHER_H__

#include "Main.h"
#include "../Matcher.h"

/*
 * "where" : [{
 *     "prop" : <property name> | [{ <property name>, ... }]
 *     "op" : "<" | "<=" | "=" | ">=" | ">" | "!="
 *     "val" : <comparison value>
 * }]
 */
class WhereMatcher: public Matcher {
public:
    WhereMatcher(const MojObject& where);
    virtual ~WhereMatcher();

    virtual bool match(const MojObject& response);

    virtual MojErr toJson(MojObject& rep, unsigned long flags) const;

protected:
    enum MatchMode {
        AndMode, OrMode
    };
    enum MatchResult {
        NoProperty, Matched, NotMatched
    };

    void validateKey(const MojObject& key) const;
    void validateOp(const MojObject& op, const MojObject& val) const;
    void validateClause(const MojObject& clause) const;
    void validateClauses(const MojObject& where) const;


    MatchResult checkClause(const MojObject& clause, const MojObject& response, MatchMode mode) const;
    MatchResult checkClauses(const MojObject& clauses, const MojObject& response, MatchMode mode) const;

    /* If a key matches a property that maps to an array, rather than a
     * specific value or object, the match proceeds to perform a DFS,
     * ultimately iterating over all the values in the array using the
     * current mode. */
    MatchResult checkProperty(const MojObject& keyArray,
                              MojObject::ConstArrayIterator keyIter,
                              const MojObject& responseArray,
                              MojObject::ConstArrayIterator responseIter,
                              const MojObject& op,
                              const MojObject& val,
                              MatchMode mode) const;
    MatchResult checkProperty(const MojObject& keyArray,
                              MojObject::ConstArrayIterator keyIter,
                              const MojObject& response,
                              const MojObject& op,
                              const MojObject& val,
                              MatchMode mode) const;
    MatchResult checkProperty(const MojObject& key,
                              const MojObject& response,
                              const MojObject& op,
                              const MojObject& val,
                              MatchMode mode) const;

    MatchResult checkMatches(const MojObject& rhsArray,
                             const MojObject& op,
                             const MojObject& val,
                             MatchMode mode) const;
    MatchResult checkMatch(const MojObject& rhs,
                           const MojObject& op,
                           const MojObject& val) const;

    MojObject m_where;
};

#endif /* __MOJO_WHERE_MATCHER_H__ */
