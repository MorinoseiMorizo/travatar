#ifndef TUNE_MERT_H__
#define TUNE_MERT_H__

#include <vector>
#include <cfloat>
#include <boost/thread.hpp>
#include <tr1/unordered_map>
#include <travatar/sparse-map.h>
#include <travatar/sentence.h>
#include <travatar/util.h>
#include <travatar/thread-pool.h>
#include <travatar/eval-measure.h>
#include <travatar/tune.h>

namespace travatar {

class TuningExample;
class TuneMert;
class OutputCollector;
class ThreadPool;

struct LineSearchResult {

    LineSearchResult() :
        pos(0.0), gain(0.0) { }
    LineSearchResult(double p, const EvalStatsPtr & b, const EvalStatsPtr & a) :
        pos(p), before(b->Clone()), after(a->Clone()), gain(a->ConvertToScore()-b->ConvertToScore()) { }
    LineSearchResult(double p, const EvalStats & b, const EvalStats & a) :
        pos(p), before(b.Clone()), after(a.Clone()), gain(a.ConvertToScore()-b.ConvertToScore()) { }

    // The gradient position
    double pos;
    // The total score before
    EvalStatsPtr before;
    // The total score after
    EvalStatsPtr after;
    // The gain between before and after
    double gain;

};

// Performs MERT
class TuneMert : public Tune {

public:

    // **** Static Utility Members ****

    // Perform line search given the current weights and gradient
    static LineSearchResult LineSearch(
      const SparseMap & weights,
      const SparseMap & gradient,
      std::vector<boost::shared_ptr<TuningExample> > & examps,
      std::pair<double,double> range = std::pair<double,double>(-DBL_MAX, DBL_MAX));

    // **** Non-static Members ****
    TuneMert() { }

    // Tune weights using MERT mert
    virtual void RunTuning();

    std::pair<double,double> FindGradientRange(WordId feat);
    std::pair<double,double> FindGradientRange(
                                const SparseMap & weights,
                                const SparseMap & gradient,
                                std::pair<double,double> range);


    // void UpdateBest(const SparseMap &gradient, const LineSearchResult &result);

protected:

};

}

#endif