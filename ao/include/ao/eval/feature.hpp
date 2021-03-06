#pragma once

#include <list>
#include <map>

#include <Eigen/Eigen>

#include "ao/eval/clause.hpp"

namespace Kernel
{

class Feature
{
public:
    struct Choice
    {
        const Clause::Id id;
        const int choice;
    };

    /*
     *  Checks to see whether a particular epsilon is compatible with
     *  all of the other epsilons in the system.
     *  This is a slow (worst-case O(n^3)) operation, but it should be called
     *  rarely and so doesn't need to be optimized yet.
     */
    bool isCompatible(Eigen::Vector3d e) const;

    /*
     *  If incompatible, does nothing and returns false
     *  Otherwise, pushes to the front of the choice list and returns true
     */
    bool push(Eigen::Vector3d e, Choice c={0, 0});

    /*
     *  Accessor method for the choice list
     */
    const std::list<Choice>& getChoices() const { return choices; }

    /*
     *  Top-level derivative (set manually)
     */
    Eigen::Vector3d deriv;

    /*
     *  Inserts a choice without any checking
     */
    void pushRaw(Choice c, Eigen::Vector3d v);

    /*
     *  Inserts a choice without an associated direction
     *  This is useful to collapse cases like min(a, a)
     */
    void pushChoice(Choice c);

    /*
     *  Appends a choice to the end of the list
     */
    void pushChoiceRaw(Choice c);

    /*
     *  Returns the epsilon associated with a particular choice
     */
    Eigen::Vector3d getEpsilon(Clause::Id i) const { return _epsilons.at(i); }

    /*
     *  Checks to see whether the given clause has an epsilon
     */
    bool hasEpsilon(Clause::Id i) const
        { return _epsilons.find(i) != _epsilons.end(); }

protected:
    /*
     *  Versions of isCompatible and push when e is known to be normalized
     */
    bool isCompatibleNorm(Eigen::Vector3d e) const;
    bool pushNorm(Eigen::Vector3d e, Choice choice);

    typedef enum { NOT_PLANAR, PLANAR_FAIL, PLANAR_SUCCESS } PlanarResult;
    PlanarResult checkPlanar(Eigen::Vector3d v) const;

    /*  Per-clause decisions  */
    std::list<Choice> choices;

    /*  Deduplicated list of epsilons  */
    std::list<Eigen::Vector3d> epsilons;

    /*  Per-clause epsilons  */
    std::map<Clause::Id, Eigen::Vector3d> _epsilons;
};

/*  Defining operator< lets us store Choices in std::set, etc */
bool operator<(const Feature::Choice& a, const Feature::Choice& b);

}   // namespace Kernel
