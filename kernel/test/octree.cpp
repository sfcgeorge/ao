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

#include "ao/kernel/tree/token.hpp"

#include "ao/kernel/render/octree.hpp"
#include "ao/kernel/render/region.hpp"

// Overloaded toString for glm::vec3
#include "glm.hpp"

TEST_CASE("Octree coordinates")
{
    Token t = Token::operation(Opcode::SUB,
              Token::operation(Opcode::ADD,
              Token::operation(Opcode::ADD, Token::operation(Opcode::MUL, Token::X(), Token::X()),
                                            Token::operation(Opcode::MUL, Token::Y(), Token::Y())),
                                            Token::operation(Opcode::MUL, Token::Z(), Token::Z())),
              Token::constant(1));

    Region r({-1, 1}, {-1, 1}, {-1, 1}, 1);
    std::unique_ptr<Octree> out(Octree::Render(t, r));
    REQUIRE(out->getType() == Octree::BRANCH);

    // Check that all child pointers are populated
    for (int i=0; i < 8; ++i)
    {
        CAPTURE(i);
        CAPTURE(out->child(i));
        REQUIRE(out->child(i) != nullptr);
    }

    // Check that Subregion::octsect and Octree::pos have consistent ordering
    for (int i=0; i < 8; ++i)
    {
        REQUIRE(out->pos(i) == out->child(i)->pos(i));
    }
}

TEST_CASE("Octree values")
{
    Token t = Token::operation(Opcode::SUB,
              Token::operation(Opcode::ADD,
              Token::operation(Opcode::ADD, Token::operation(Opcode::MUL, Token::X(), Token::X()),
                                            Token::operation(Opcode::MUL, Token::Y(), Token::Y())),
                                            Token::operation(Opcode::MUL, Token::Z(), Token::Z())),
              Token::constant(1));

    Region r({-1, 1}, {-1, 1}, {-1, 1}, 1);
    REQUIRE(r.X.values.size() == 2);

    std::unique_ptr<Octree> out(Octree::Render(t, r));

    // Check that values and gradients are correct
    for (int i=0; i < 8; ++i)
    {
        REQUIRE(!out->corner(i));
    }
}

TEST_CASE("Vertex positioning")
{
    Token t = Token::operation(Opcode::SUB,
              Token::operation(Opcode::ADD,
              Token::operation(Opcode::ADD, Token::operation(Opcode::MUL, Token::X(), Token::X()),
                                            Token::operation(Opcode::MUL, Token::Y(), Token::Y())),
                                            Token::operation(Opcode::MUL, Token::Z(), Token::Z())),
               Token::constant(0.5));

    Region r({-1, 1}, {-1, 1}, {-1, 1}, 4);

    std::unique_ptr<Octree> out(Octree::Render(t, r));

    // Walk every leaf node in the octree, keeping track of the
    // minimum and maximum vertex radius
    float rmax = -std::numeric_limits<float>::infinity();
    float rmin =  std::numeric_limits<float>::infinity();

    // Queue of octrees to process
    std::list<const Octree*> targets = {out.get()};
    while (targets.size())
    {
        const Octree* o = targets.front();
        targets.pop_front();

        if (o->getType() == Octree::BRANCH)
        {
            for (unsigned i=0; i < 8; ++i)
            {
                targets.push_back(o->child(i));
            }
        }
        else if (o->getType() == Octree::LEAF)
        {
            float r = glm::length(o->getVertex());
            rmax = std::max(r, rmax);
            rmin = std::max(r, rmin);
        }
    }

    REQUIRE(rmin > sqrt(0.5)*0.95);
    REQUIRE(rmax < sqrt(0.5)*1.05);
}
