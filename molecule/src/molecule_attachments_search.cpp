﻿/****************************************************************************
* Copyright (C) 2009-2015 EPAM Systems
*
* This file is part of Indigo toolkit.
*
* This file may be distributed and/or modified under the terms of the
* GNU General Public License version 3 as published by the Free Software
* Foundation and appearing in the file LICENSE.GPL included in the
* packaging of this file.
*
* This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
* WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
***************************************************************************/

#include "base_cpp/array.h"
#include "molecule/molecule_attachments_search.h"

using namespace indigo;

IMPL_ERROR(Topology, "topology");
CP_DEF(Topology);

IMPL_ERROR(IntervalFilter, "interval filter");
CP_DEF(IntervalFilter);

IMPL_ERROR(OccurrenceRestrictions, "occurrence restrictions");
CP_DEF(OccurrenceRestrictions);

IMPL_ERROR(ErrorThrower, "attachments search");
CP_DEF(Attachments);

int  MultiMap::size() const {
    return map.size();
}

void MultiMap::insert(int key, int value) {
    Set* *ptr = map.at2(key);
    if (ptr != nullptr) {
        (*ptr)->insert(value);
        return;
    }

    Set *values = new Set();
    map.insert(key, values);
    values->insert(value);
}

void MultiMap::insert(int key, const Array<int> &values) {
    for (auto i = 0; i < values.size(); i++) {
        insert(key, values[i]);
    }
}

void MultiMap::remove(int key, int value) {
    Set* *ptr = map.at2(key);
    if (ptr == nullptr) {
        return;
    }

    Set *values = (*ptr);
    values->remove_if_exists(value);
    if (values->size() == 0) {
        map.remove(key);
        delete values;
    }
}

void MultiMap::remove(int key) {
    if (map.find(key)) {
        map.remove(key);
    }
}

const char* MultiMap::print() const {
    return print("->");
}

const char* MultiMap::print(const char *delim) const {
    return indigo::print(map, delim);
}

const Set& MultiMap::operator[](int key) const {
    Set* *ptr = map.at2(key);
    if (ptr == nullptr) return nil;
    return **ptr;
}

const Set& MultiMap::nil = Set();

const char* Topology::print() const {
    const char* sf = forward.print("->");
    const char* sb = backward.print("<-");
    const char* sc = indigo::print(&current);

    char* ptr = new char[strlen(sc) + strlen(sf) + strlen(sb) + 255];
    sprintf(ptr, "<current = %s;\n  forward  = %s;\n  backward = %s>", sc, sf, sb);
    return ptr;
}

const Path &Topology::history() const {
    Path *result = new Path();
    result->copy(path);
    return *result;
}

const Set& Topology::pending() const {
    return current;
}

const Set& Topology::satisfied() const {
    return used;
}

void Topology::depends(Node source, Node target) {
    expand(std::max(source, target));

    if (path.size() > 0) {
        throw Error("It is impossible to change topology"
            "after you started to resolve dependencies");
    }

    current.remove_if_exists(target);
    backward.insert(target, source);
    forward.insert(source, target);
}

bool Topology::satisfy(Node source) {
    if (!current.find(source)) {
        return false;
    }
    current.remove(source);
    used.insert(source);

    const Set &targets = forward[source];
    for (int i = targets.begin(); i != targets.end(); i = targets.next(i)) {
        Node target = targets.key(i);
        backward.remove(target, source);
        if (backward[target].size() < 1) {
            current.insert(target);
        }
    }
    forward.remove(source);
    path.push(source);
    return true;
}

bool Topology::finished() const {
    printf("Topology::finished(): path.size() == %d; lim == %d; current.size() == %d\n", path.size(), lim, current.size());
    const bool x = path.size() == lim;
    const bool y = current.size() < 1;
    printf("Topology::finished(): x == %d; y == %d; (x == y) == %d; (x || y ? x == y : true) == %d\n", x, y, x == y, x || y ? x == y : true);
    assert(x || y ? x == y : true);
    return x && y;
}

void Topology::expand(int nlim) {
    for (int i = lim + 1; i <= nlim; i++) {
        current.insert(i);
    }
    lim = std::max(lim, nlim);
}

IntervalFilter::IntervalFilter(const Array<int> &array) :
CP_INIT
{
    map.copy(array);
    const int n = map.size();
    assert(n % 2 == 0);
    assert(n > 1);
}

IntervalFilter::IntervalFilter(const IntervalFilter &other) :
  IntervalFilter(other.map)
{ };

IntervalFilter::IntervalFilter(int from, int to) :
CP_INIT
{
    if (from > to) throw Error("Interval [%d..%d] is forbidden", from, to);
    map.push(from);
    map.push(to);
}

IntervalFilter::IntervalFilter(int bits) :
  IntervalFilter(bits >> 16, bits & 0x0000ffff)
{ }

// + if the nearest point is on the right,
// - if on the left
int IntervalFilter::distance(int x) const {
    int x1_;
    for (int i = 0; i < map.size(); i += 2) {
        int x0 = map[i], x1 = map[i + 1]; x1_ = -x0;
        if (x1_ < x && x < x0) {
            const int l = x1_ - x, r = x0 - x;
            return -l < r ? l : r ;
        }
        if (x0 <= x && x <= x1) {
            return 0;
        }
        x1_ = x1;
    }
    assert(x1_ < x);
    return x1_ - x;
}

IntervalFilter IntervalFilter::join(const IntervalFilter &other) const {
    Array<int> merged;

    const Array<int> *xs = &map, *ys = &other.map;
    int n = xs->size(), m = ys->size();
    assert(n % 2 == 0 && m % 2 == 0);
    assert(n > 1 && m > 1);

    int    i = 0,   j = 0;
    while (i < n && j < m) {
        int x0 = (*xs)[i], x1 = (*xs)[i + 1],
            y0 = (*ys)[j], y1 = (*ys)[j + 1];
        assert(x0 <= x1);
        assert(y0 <= y1);
        swap5(xs, x0, x1, i, n,
              ys, y0, y1, j, m,
              y1 >= x1);
        if (x1 < y0) {
            merged.push(x0);
            merged.push(x1);
            i += 2;
        } else {
            merged.push(min4(x0, x1, y0, y1));
            merged.push(max4(x0, x1, y0, y1));
            i += 2; j += 2;
        }
    }
    if (i < n) { assert(j >= m); append(merged, *xs, i); }
    else { if (j < m) { append(merged, *ys, j); } }

    return IntervalFilter(merged);
}

void IntervalFilter::append(Array<int> &target, const Array<int> &source, int from) {
    const int n = source.size();
    assert((n - from) % 2 == 0);
    assert(from < n);

    for (int i = from; i < n; i++) {
        target.push(source[i]);
    }
}

const char* IntervalFilter::print() const {
    char* ptr = new char[1024];
    const char* out = ptr;

    bool empty = true;
    for (int i = 0; i < map.size(); i += 2) {
        const char* d = empty ? "" : " \\/ ";
        empty = false;

        int x0 = map[i], x1 = map[i+1];
        if (x0 == x1) { ptr += sprintf(ptr, "%s{%d}", d, x0); }
        else { ptr += sprintf(ptr, "%s[%d..%d]", d, x0, x1); };
    }

    return out;
}

const IntervalFilter OccurrenceRestrictions::DEFAULT = gt(0);

const char* OccurrenceRestrictions::print() const {
    return indigo::print(restrictions);
}

void OccurrenceRestrictions::free(int group) {
    set(group, DEFAULT);
}

void OccurrenceRestrictions::set(int group, const IntervalFilter &f) {
    restrictions[group-1] = f;
}

void OccurrenceRestrictions::set(int group, const Array<int> &bits) {
    IntervalFilter f(bits[0]);
    for (auto i = 1; 1 < bits.size(); i++) {
        f = f && IntervalFilter(bits[i]);
    }
    set(group, f);
}