#ifndef PS_ADAM_H
#define PS_ADAM_H
#include "ps.h"
#include "random.h"
#include "llog.h"
#include "lutil.h"
#include <pthread.h>
#include <math.h>
#include <assert.h>
#include <tr1/unordered_map>
#include <fstream>
#include <sstream>

/*  Authors: */
/*      Tao Hui <taohui3@gmail.com> */

struct ADAMParam {
    ADAMParam() : beta1(0.9f), beta2(0.999f),
        epsilon(1e-8), alpha(0.001f), 
        l1(0.0001f), l2(0.0002), init_stdev(0.01f) {}
    double beta1;
    double beta2;
    double epsilon;
    double alpha;
    double l1;
    double l2;
    double init_stdev;
};

struct ADAMEntry {
    ADAMEntry() : x(0.0), m(0.0), v(0.0), g(0.0), t(0.0) {}
    double x;
    double m;
    double v;
    double g;
    double t;

    friend std::ostream& operator <<(std::ostream& os, const ADAMEntry& entry);
    friend std::istream& operator >>(std::istream& is, ADAMEntry& entry);
};

std::ostream& operator <<(std::ostream& os, const ADAMEntry& entry)
{
    os << entry.x << '\t' << entry.m << '\t' << 
        entry.v << '\t' << entry.g << '\t' << entry.t;
    return os;
}

std::istream& operator >>(std::istream& is, ADAMEntry& entry)
{
    is >> entry.x >> entry.m >> entry.v >> entry.g >> entry.t;
    return is;
}

class ADAMParameterServer : public ParameterServer {
public:
    ADAMParameterServer(ADAMParam param, llog_t *llog, const char *home_path) : 
        ParameterServer(llog, home_path), param_(param) {
    }
    ~ADAMParameterServer() {
    }

    double get(uint64_t key) {
        double ret = 0.0;
        bool found = false;
#ifdef USE_TBB
        tbb::concurrent_hash_map<uint64_t, ADAMEntry, UInt64Compare>::const_accessor it;
        if (weights_.find(it, key)) {
            ret = it->second.x;
            found = true;
        }
        it.release();
#else
        std::tr1::unordered_map<uint64_t, ADAMEntry>::iterator it;
        if ((it = weights_.find(key)) != weights_.end()) {
            ret = it->second.x;
            found = true;
        }
#endif
        return ret;
        if (!found) {
            ret = ran_gaussian(0.0, param_.init_stdev);
            ADAMEntry entry;
            entry.x = ret;
#ifdef USE_TBB
            tbb::concurrent_hash_map<uint64_t, ADAMEntry, UInt64Compare>::accessor it2;
            if (weights_.insert(it2, key)) {
                it2->second = entry;
            } else {
                ret = it2->second.x;
            }
            it2.release();
#else 
            weights_.insert(std::pair<uint64_t, ADAMEntry>(key, entry));
#endif
        }
        return ret;
    }

    void update_adam_entry(ADAMEntry& entry, double g) {
        entry.g = g;
        entry.t += 1;
        entry.m = param_.beta1 * entry.m + (1.0f - param_.beta1) * g;
        entry.v = param_.beta2 * entry.v + (1.0f - param_.beta2) * g * g;

        double m = entry.m / (1.0f - pow(param_.beta1, entry.t));
        double v = entry.v / (1.0f - pow(param_.beta2, entry.t));
        entry.x -= param_.alpha * m / (sqrt(v) + param_.epsilon);
    }

    void update(uint64_t key, double g, time_t timestamp) {

#ifdef USE_TBB
        tbb::concurrent_hash_map<uint64_t, ADAMEntry, UInt64Compare>::accessor it;
        if (weights_.insert(it, key))
        {
#else
        std::tr1::unordered_map<uint64_t, ADAMEntry>::iterator it;
        if ((it = weights_.find(key)) == weights_.end())
        {
#endif
            ADAMEntry entry;
            update_adam_entry(entry, g);
#ifdef USE_TBB
            it->second = entry;
#else
            weights_.insert(std::pair<uint64_t, ADAMEntry>(key, entry));
#endif
        }
        else
        {
            update_adam_entry(it->second, g);
        }
#ifdef USE_TBB
        it.release();
#endif
    }

    double xnorm() {
        double s = 0.0;
#ifdef USE_TBB
        tbb::concurrent_hash_map<uint64_t, ADAMEntry, UInt64Compare>::const_iterator it;
#else
        std::tr1::unordered_map<uint64_t, ADAMEntry>::iterator it;
#endif
        for (it = weights_.begin(); it != weights_.end(); ++it) {
            s += it->second.x * it->second.x;
        }
        return sqrt(s);
    }

    double gnorm() {
        double s = 0.0;
#ifdef USE_TBB
        tbb::concurrent_hash_map<uint64_t, ADAMEntry, UInt64Compare>::const_iterator it;
#else
        std::tr1::unordered_map<uint64_t, ADAMEntry>::iterator it;
#endif
        for (it = weights_.begin(); it != weights_.end(); ++it) {
            s += it->second.g * it->second.g;
        }
        return sqrt(s);
    }

    int write_weights(const char *filename) {
        char filepath[1024];
        snprintf(filepath, 1024, "%s/%s", home_path_.c_str(), filename);
        std::ofstream out(filepath);
        if (!out) {
            return -1;
        }
    
#ifdef USE_TBB
        tbb::concurrent_hash_map<uint64_t, ADAMEntry, UInt64Compare>::const_iterator it;
#else
        std::tr1::unordered_map<uint64_t, ADAMEntry>::iterator it;
#endif
        for (it = weights_.begin(); it != weights_.end(); ++it) {
            if (it->second.x) out << it->first << " : " << it->second.x << std::endl;
        }
        out.close();
    
        return 0;
    
    }

    int write_state(const char *filename) {
        char filepath[1024];
        snprintf(filepath, 1024, "%s/%s", home_path_.c_str(), filename);
        std::ofstream out(filepath);
        if (!out) {
            return -1;
        }
    
#ifdef USE_TBB
        tbb::concurrent_hash_map<uint64_t, ADAMEntry, UInt64Compare>::const_iterator it;
#else
        std::tr1::unordered_map<uint64_t, ADAMEntry>::iterator it;
#endif
        for (it = weights_.begin(); it != weights_.end(); ++it) {
            out << it->first << '\t' << it->second << std::endl;
        }
        out.close();

        return 0;
    }

    int read_state(const char *filename) {
        char filepath[1024];
        snprintf(filepath, 1024, "%s/%s", home_path_.c_str(), filename);
        std::ifstream in(filepath);
        if (!in) {
            return -1;
        }
    
        uint64_t index;
        ADAMEntry entry;
#ifdef USE_TBB
        tbb::concurrent_hash_map<uint64_t, ADAMEntry, UInt64Compare>::accessor it;
#endif
        while (in >> index >> entry) {
#ifdef USE_TBB
            weights_.insert(it, index);
            it->second = entry;
            it.release();
#else
            weights_.insert(std::pair<uint64_t, ADAMEntry>(index, entry));;
#endif
        }
        in.close();

        return 0;
    }


    void reset() { weights_.clear(); }

private:
    ADAMParam param_;
#ifdef USE_TBB
    tbb::concurrent_hash_map<uint64_t, ADAMEntry, UInt64Compare> weights_;
#else
    std::tr1::unordered_map<uint64_t, ADAMEntry> weights_;
#endif
};

#endif
