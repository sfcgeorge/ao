#include <Eigen/Geometry>

#include "catch.hpp"

#include "ao/tree/tree.hpp"
#include "ao/eval/evaluator.hpp"

#include "util/shapes.hpp"

using namespace Kernel;

TEST_CASE("Principle variable evaluation")
{
    SECTION("X")
    {
        Evaluator e(Tree::X());
        REQUIRE(e.eval({1.0, 2.0, 3.0}) == 1.0);
    }

    SECTION("Y")
    {
        Evaluator e(Tree::Y());
        REQUIRE(e.eval({1.0, 2.0, 3.0}) == 2.0);
    }
}

TEST_CASE("Constant evaluation")
{
    Evaluator e(Tree(3.14));
    REQUIRE(e.eval({1.0, 2.0, 3.0}) == Approx(3.14));
}

TEST_CASE("Secondary variable evaluation")
{
    auto v = Tree::var();
    Evaluator e(v, {{v.id(), 3.14}});
    REQUIRE(e.eval({1.0, 2.0, 3.0}) == Approx(3.14));
}

TEST_CASE("Evaluator::gradient")
{
    SECTION("constant + variable")
    {
        auto v = Tree::var();
        Evaluator e(v + 1.0, {{v.id(), 3.14}});
        REQUIRE(e.eval({1.0, 2.0, 3.0}) == Approx(4.14));
        auto g = e.gradient({1, 2, 3});
        REQUIRE(g.size() == 1);
        REQUIRE(g.count(v.id()) == 1);
        REQUIRE(g.at(v.id()) == Approx(1));
    }

    SECTION("x * variable")
    {
        auto v = Tree::var();
        Evaluator e(Tree::X() * v, {{v.id(), 1}});
        {
            auto g = e.gradient({2, 0, 0});
            REQUIRE(g.size() == 1);
            REQUIRE(g.at(v.id()) == Approx(2));
        }
        {
            auto g = e.gradient({3, 0, 0});
            REQUIRE(g.at(v.id()) == Approx(3));
        }
    }

    SECTION("Multiple variables")
    {
        // Deliberately construct out of order
        auto a = Tree::var();
        auto c = Tree::var();
        auto b = Tree::var();

        Evaluator e(Tree(a*1 + b*2 + c*3),
                {{a.id(), 3}, {c.id(), 7}, {b.id(), 5}});
        REQUIRE(e.eval({0, 0, 0}) == Approx(34));

        auto g = e.gradient({0, 0, 0});
        REQUIRE(g.at(a.id()) == Approx(1.0f));
        REQUIRE(g.at(b.id()) == Approx(2.0f));
        REQUIRE(g.at(c.id()) == Approx(3.0f));
    }
}

TEST_CASE("Evaluator::setVar")
{
    // Deliberately construct out of order
    auto a = Tree::var();
    auto c = Tree::var();
    auto b = Tree::var();

    Evaluator e(a*1 + b*2 + c*3,
                {{a.id(), 3}, {c.id(), 7}, {b.id(), 5}});
    REQUIRE(e.eval({0, 0, 0}) == Approx(34));

    e.setVar(a.id(), 5);
    REQUIRE(e.eval({0, 0, 0}) == Approx(36));
    e.setVar(b.id(), 0);
    REQUIRE(e.eval({0, 0, 0}) == Approx(26));
    e.setVar(c.id(), 10);
    REQUIRE(e.eval({0, 0, 0}) == Approx(35));
}

TEST_CASE("Evaluator::varValues")
{
    // Deliberately construct out of order
    auto a = Tree::var();
    Evaluator e(a, {{a.id(), 3}});

    {
        auto v = e.varValues();
        REQUIRE(v.size() == 1);
        REQUIRE(v.at(a.id()) == 3.0);
    }

    e.setVar(a.id(), 5.0);
    {
        auto v = e.varValues();
        REQUIRE(v.size() == 1);
        REQUIRE(v.at(a.id()) == 5.0);
    }
}

TEST_CASE("Float evaluation")
{
    SECTION("X + 1")
    {
        Evaluator e(Tree::X() + 1);
        REQUIRE(e.eval({1.0, 2.0, 3.0}) == 2.0);
    }

    SECTION("X + Z")
    {
        Evaluator e(Tree::X() + Tree::Z());
        REQUIRE(e.eval({1.0, 2.0, 3.0}) == 4.0);
    }
}

TEST_CASE("Interval evaluation")
{
    Evaluator e(Tree::X() + 1);

    auto out = e.eval({1,1,1}, {2,2,2});

    REQUIRE(out.lower() == 2.0);
    REQUIRE(out.upper() == 3.0);
}

TEST_CASE("Push / pop behavior")
{
    Evaluator e(min(Tree::X() + 1, Tree::Y() + 1));

    // Store -3 in the rhs's value
    REQUIRE(e.eval({1.0f, -3.0f, 0.0f}) == -2);

    // Do an interval evaluation that will lead to disabling the rhs
    auto i = e.eval({-5, 8, 0}, {-4, 9, 0});
    REQUIRE(i.lower() == -4);
    REQUIRE(i.upper() == -3);

    // Push (which should disable the rhs of min
    e.push();

    // Check to make sure that the push disabled something
    CAPTURE(e.utilization());
    REQUIRE(e.utilization() < 1);

    // Require that the evaluation gets 1
    REQUIRE(e.eval({1.0f, 2.0f, 0.0f}) == 2);
}

TEST_CASE("Evaluator::derivs")
{
    SECTION("X")
    {
        Evaluator e(Tree::X());
        e.set({0, 0, 0}, 0);
        e.set({1, 2, 3}, 1);
        auto d = e.derivs(2);

        // Values = x
        REQUIRE(d.v[0] == 0.0);
        REQUIRE(d.v[1] == 1.0);

        // d/dx = 1
        REQUIRE(d.d.col(0).x() == 1.0);
        REQUIRE(d.d.col(1).x() == 1.0);

        // d/dy = 0
        REQUIRE(d.d.col(0).y() == 0.0);
        REQUIRE(d.d.col(1).y() == 0.0);

        // d/dz = 0
        REQUIRE(d.d.col(0).z() == 0.0);
        REQUIRE(d.d.col(1).z() == 0.0);
    }

    SECTION("X + Z")
    {
        Evaluator e(Tree::X() + Tree::Z());

        e.set({1, 1, 1}, 0);
        e.set({1, 2, 3}, 1);
        auto d = e.derivs(2);

        // Values = x
        REQUIRE(d.v[0] == 2.0);
        REQUIRE(d.v[1] == 4.0);

        // d/dx = 1
        REQUIRE(d.d.col(0).x() == 1.0);
        REQUIRE(d.d.col(1).x() == 1.0);

        // d/dy = 0
        REQUIRE(d.d.col(0).y() == 0.0);
        REQUIRE(d.d.col(1).y() == 0.0);

        // d/dz = 1
        REQUIRE(d.d.col(0).z() == 1.0);
        REQUIRE(d.d.col(1).z() == 1.0);
    }
}

TEST_CASE("Evaluator::specialize")
{
    Evaluator e(min(Tree::X(), Tree::Y()));

    e.specialize({-1, 0, 0}); // specialize to just "X"
    REQUIRE(e.eval({-2, 0, 0}) == -2);
    REQUIRE(e.eval({4, 0, 0}) == 4);
    REQUIRE(e.eval({4, 5, 0}) == 4);
    REQUIRE(e.eval({10, 5, 0}) == 10);

    e.pop();
    e.specialize({0, -1, 0}); // specialize to just "X"
    REQUIRE(e.eval({-2, 0, 0}) == 0);
    REQUIRE(e.eval({4, 0, 0}) == 0);
    REQUIRE(e.eval({4, 5, 0}) == 5);
    REQUIRE(e.eval({10, 5, 0}) == 5);
}

TEST_CASE("Evaluator::isInside")
{
    SECTION("Single plane edge")
    {
        Evaluator a(Tree::X());
        REQUIRE(a.isInside({0, 0, 0}) == true);
        REQUIRE(a.isInside({-1, 0, 0}) == true);
        REQUIRE(a.isInside({1, 0, 0}) == false);
    }

    SECTION("2D plane-to-plane (full)")
    {
        Evaluator b(min(Tree::X(), -Tree::X()));
        REQUIRE(b.isInside({0, 0, 0}) == true);
        REQUIRE(b.isInside({1, 0, 0}) == true);
        REQUIRE(b.isInside({-1, 0, 0}) == true);
    }

    SECTION("2D plane-to-plane (empty)")
    {
        Evaluator c(max(Tree::X(), -Tree::X()));
        REQUIRE(c.isInside({0, 0, 0}) == false);
        REQUIRE(c.isInside({1, 0, 0}) == false);
        REQUIRE(c.isInside({-1, 0, 0}) == false);
    }

    SECTION("2D Corner")
    {
        Evaluator d(min(min(Tree::X(), -Tree::X()), min(Tree::Y(), -Tree::Y())));
        REQUIRE(d.isInside({0, 0, 0}) == true);
    }
}

TEST_CASE("Evaluator::isAmbiguous")
{
    SECTION("Single feature")
    {
        Evaluator e(Tree::X());
        REQUIRE(e.isAmbiguous({0, 0, 0}) == false);
        REQUIRE(e.isAmbiguous({1, 0, 0}) == false);
    }

    SECTION("Two features (min)")
    {
        Evaluator e(min(Tree::X(), -Tree::X()));
        REQUIRE(e.isAmbiguous({0, 0, 0}) == true);
        REQUIRE(e.isAmbiguous({1, 0, 0}) == false);
    }
}

TEST_CASE("Evaluator::featuresAt")
{
    SECTION("Single feature")
    {
        Evaluator e(Tree::X());
        auto fs = e.featuresAt({0, 0, 0});
        REQUIRE(fs.size() == 1);
        REQUIRE(fs.front().deriv == Eigen::Vector3d(1, 0, 0));
    }

    SECTION("Two features (min)")
    {
        Evaluator e(min(Tree::X(), -Tree::X()));
        auto fs = e.featuresAt({0, 0, 0});
        REQUIRE(fs.size() == 2);
        auto i = fs.begin();
        REQUIRE((i++)->deriv == Eigen::Vector3d(1, 0, 0));
        REQUIRE((i++)->deriv == Eigen::Vector3d(-1, 0, 0));
    }

    SECTION("Two features (max)")
    {
        Evaluator e(max(Tree::X(), -Tree::X()));
        auto fs = e.featuresAt({0, 0, 0});
        REQUIRE(fs.size() == 2);
        auto i = fs.begin();
        REQUIRE((i++)->deriv == Eigen::Vector3d(1, 0, 0));
        REQUIRE((i++)->deriv == Eigen::Vector3d(-1, 0, 0));
    }

    SECTION("Three features")
    {
        Evaluator e(min(Tree::X(), min(Tree::Y(), Tree::Z())));
        auto fs = e.featuresAt({0, 0, 0});

        // TODO: This should actually only give 3 features, because the branches
        // that chooise X, Y and X, Z collapse to X.
        REQUIRE(fs.size() == 3);
        auto i = fs.begin();
        REQUIRE((i++)->deriv == Eigen::Vector3d(1, 0, 0));
        REQUIRE((i++)->deriv == Eigen::Vector3d(0, 1, 0));
        REQUIRE((i++)->deriv == Eigen::Vector3d(0, 0, 1));
    }

    SECTION("Buried ambiguity")
    {
        // The ambiguity here (in max(-1 - X, X) is irrelevant, as
        // it ends up being masked by the Y clause)
        Evaluator e(rectangle(-1, 0, -1, 1));
        REQUIRE(e.featuresAt({-0.5, -1, 0}).size() == 1);
    }

    SECTION("One feature (nested)")
    {
        auto t = max(max(max(-Tree::X(), Tree::X() - 1),
                         max(-Tree::Y(), Tree::Y() - 1)),
                    -Tree::X());
        Evaluator e(t);
        REQUIRE(e.featuresAt({0, 0.2, 0}).size() == 1);
    }

    SECTION("One feature (duplicated)")
    {
        auto t = max(Tree::X(), Tree::X());
        Evaluator e(t);
        REQUIRE(e.featuresAt({0, 0.2, 0}).size() == 1);
    }

    SECTION("One feature (duplicated multiple times)")
    {
        auto t = max(Tree::X(), max(Tree::X(), Tree::X()));
        Evaluator e(t);
        REQUIRE(e.featuresAt({0, 0.2, 0}).size() == 1);
    }

    SECTION("One feature (duplicated even more times)")
    {
        auto t = max(max(Tree::X(), Tree::X()), max(Tree::X(), Tree::X()));
        Evaluator e(t);
        REQUIRE(e.featuresAt({0, 0.2, 0}).size() == 1);
    }

    SECTION("Coincident planes with same normal")
    {
        auto t = max(Tree::Z() - 6, Tree::Z() + -6);
        Evaluator e(t);
        REQUIRE(e.featuresAt({0, 0, 6}).size() == 1);
    }
}

TEST_CASE("Evaluator::getAmbiguous")
{
    Evaluator e(min(Tree::X(), -Tree::X()));
    e.set({0, 0, 0}, 0);
    e.set({1, 0, 0}, 1);
    e.set({2, 0, 0}, 2);
    e.set({0, 0, 0}, 3);

    e.values(4);

    auto a = e.getAmbiguous(3);
    REQUIRE(a.count() == 1);
    REQUIRE(a(0) == 1);

    auto b = e.getAmbiguous(4);
    REQUIRE(b.count() == 2);
    REQUIRE(b(0) == 1);
    REQUIRE(b(3) == 1);
}

TEST_CASE("Evaluator::push(Feature)")
{
    Evaluator e(min(Tree::X(), -Tree::X()));
    REQUIRE(e.eval({0, 0, 0}) == 0); // Force an ambiguous evaluation
    Feature f;

    SECTION("LHS")
    {   // Use a dummy feature to select the first branch
        REQUIRE(f.push({1, 0, 0}, {1, 0}));
        e.push(f);
        REQUIRE(e.eval({1, 0, 0}) == 1);
        REQUIRE(e.utilization() < 1);
    }

    SECTION("RHS")
    {   // Use a dummy feature to select the second branch
        REQUIRE(f.push({-1, 0, 0}, {1, 1}));
        e.push(f);
        REQUIRE(e.eval({-2, 0, 0}) == 2);
        REQUIRE(e.utilization() < 1);
    }
}

TEST_CASE("Evaluator::eval (every operation)")
{
    for (unsigned i=7; i < Kernel::Opcode::LAST_OP; ++i)
    {
        auto op = (Kernel::Opcode::Opcode)i;
        Tree t = (Opcode::args(op) == 2 ? Tree(op, Tree::X(), Tree(5))
                                        : Tree(op, Tree::X()));
        Evaluator e(t);
        e.eval({0, 0, 0});
        e.derivs(1);
        e.set({0, 0, 0}, {1, 1, 1});
        e.interval();
        REQUIRE(true /* No crash! */ );
    }
}
