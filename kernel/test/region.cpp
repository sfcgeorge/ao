/*
 *  Copyright (C) 2016 Matthew Keeter  <matt.j.keeter@gmail.com>
 *
 *  This file is part of the Ao library.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  Ao is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with Ao.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <catch/catch.hpp>

#include "ao/kernel/render/region.hpp"

TEST_CASE("Region::Axis construction")
{
    SECTION("Exact values")
    {
        auto da = Region::Axis(Interval(0, 1), 1.0f);
        REQUIRE(da.values.size() == 1);

        auto db = Region::Axis(Interval(0, 1), 10.0f);
        REQUIRE(db.values.size() == 10);

        auto dc = Region::Axis(Interval(0, 0), 1.0f);
        REQUIRE(dc.values.size() == 1);

        auto dd = Region::Axis(Interval(-1, 1), 0.0f);
        REQUIRE(dd.values.size() == 1);
    }

    SECTION("Expanding interval")
    {
        auto da = Region::Axis(Interval(0, 1.1), 1.0f);
        REQUIRE(da.values.size() == 2);
    }
}

TEST_CASE("Region::Axis values")
{
    SECTION("Exact values");
    {
        auto da = Region::Axis(Interval(0, 1), 1.0f);
        REQUIRE(da.values[0] == 0.5);

        auto db = Region::Axis(Interval(-0.5, 0.5), 3.0f);
        REQUIRE(db.values[1] == 0);

        auto dc = Region::Axis(Interval(-1, 1), 0.0f);
        REQUIRE(dc.values[0] == 0);
    }

    SECTION("Expanding interval")
    {
        auto da = Region::Axis(Interval(0, 1.2), 1.0f);
        REQUIRE(da.bounds.lower() == Approx(-0.4f));
        REQUIRE(da.bounds.upper() == Approx(1.6f));
        REQUIRE(da.values[0] == Approx(0.1f));
        REQUIRE(da.values[1] == Approx(1.1f));
    }
}

TEST_CASE("Region resolution")
{
    auto r = Region({0, 10}, {0, 5}, {0, 2.5}, 10);

    REQUIRE(r.X.values.size() == 100);
    REQUIRE(r.Y.values.size() == 50);
    REQUIRE(r.Z.values.size() == 25);
}

TEST_CASE("Region power-of-two generation (cube)")
{
    auto r = Region({0, 10}, {0, 5}, {0, 2.5}, 10);
    auto p = r.powerOfTwo(3);

    REQUIRE(p.X.values.size() == 128);
    REQUIRE(p.X.values.size() == p.Y.values.size());
    REQUIRE(p.X.values.size() == p.Z.values.size());

    REQUIRE(p.X.bounds.lower() <= r.X.bounds.lower());
    REQUIRE(p.X.bounds.upper() >= r.X.bounds.upper());

    REQUIRE(p.Y.bounds.lower() <= r.Y.bounds.lower());
    REQUIRE(p.Y.bounds.upper() >= r.Y.bounds.upper());

    REQUIRE(p.Z.bounds.lower() <= r.Z.bounds.lower());
    REQUIRE(p.Z.bounds.upper() >= r.Z.bounds.upper());
}

TEST_CASE("Region power-of-two generation (square)")
{
    auto r = Region({0, 10}, {0, 5}, {0, 0}, 10);
    auto p = r.powerOfTwo(2);

    REQUIRE(p.X.values.size() == 128);
    REQUIRE(p.X.values.size() == p.Y.values.size());
    REQUIRE(p.Z.values.size() == 1);

    REQUIRE(p.X.bounds.lower() <= r.X.bounds.lower());
    REQUIRE(p.X.bounds.upper() >= r.X.bounds.upper());

    REQUIRE(p.Y.bounds.lower() <= r.Y.bounds.lower());
    REQUIRE(p.Y.bounds.upper() >= r.Y.bounds.upper());

    REQUIRE(p.Z.bounds.lower() == r.Z.bounds.lower());
    REQUIRE(p.Z.bounds.upper() == r.Z.bounds.upper());
}
