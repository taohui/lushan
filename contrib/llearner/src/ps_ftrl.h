#ifndef PS_FTRL_H
#define PS_FTRL_H
#include "ps.h"
#include "llog.h"
#include "lutil.h"
#include <math.h>
#include <assert.h>
#include <tr1/unordered_map>

#ifdef USE_TBB
#include "tbb/concurrent_hash_map.h"
#endif

#include <fstream>
#include <sstream>

/*  Authors: */
/*      Tao Hui <taohui3@gmail.com> */

struct FTRLParam {
    FTRLParam() : l1(0.0001), l2(0.0002), 
        alpha(0.3), beta(1.0){}

    double l1;
    double l2;
    double alpha;
    double beta;
};

struct FTRLEntry {
    FTRLEntry() : x(0.0), n(0.0), z(0.0), g(0.0), t(0) {}
    double x;
    double n;
    double z;
    double g;
    time_t t;
    friend std::ostream& operator <<(std::ostream& os, const FTRLEntry& entry);
    friend std::istream& operator >>(std::istream& is, FTRLEntry& entry);
};

std::ostream& operator <<(std::ostream& os, const FTRLEntry& entry)
{
    os << entry.x << '\t' << entry.n << '\t' << entry.z << '\t' << entry.g << '\t' << entry.t;
    return os;
}

std::istream& operator >>(std::istream& is, FTRLEntry& entry)
{
    is >> entry.x >> entry.n >> entry.z >> entry.g >> entry.t;
    return is;
}

#ifdef USE_TBB

struct UInt64Compare {
    static size_t hash(const uint64_t x) { return x; }
    static bool equal(const uint64_t x, const uint64_t y) { return x == y; }
};

#endif

class FTRLParameterServer : public ParameterServer {
public:
    FTRLParameterServer(FTRLParam param, llog_t *llog, const char *home_path) : 
        ParameterServer(llog, home_path), param_(param) {
    }
    ~FTRLParameterServer() {
    }

    double get(uint64_t key) {
        double ret = 0.0;
#ifdef USE_TBB
        tbb::concurrent_hash_map<uint64_t, FTRLEntry, UInt64Compare>::const_accessor it;
        if (weights_.find(it, key)) {
            ret = it->second.x;
        }
        it.release();
#else
        std::tr1::unordered_map<uint64_t, FTRLEntry>::iterator it;
        if ((it = weights_.find(key)) != weights_.end()) {
            ret = it->second.x;
        }
#endif
        return ret;
    }

    void update(uint64_t key, double g, time_t timestamp) {

#ifdef USE_TBB
        tbb::concurrent_hash_map<uint64_t, FTRLEntry, UInt64Compare>::accessor it;
        if (weights_.insert(it, key))
        {
#else
        std::tr1::unordered_map<uint64_t, FTRLEntry>::iterator it;
        if ((it = weights_.find(key)) == weights_.end())
        {
#endif
            double sigma = g / param_.alpha;
            FTRLEntry entry;
            entry.g = g;
            entry.z += g - sigma * entry.x;
            entry.n += g * g;
            if (fabs(entry.z) >= param_.l1)
            {
                entry.x = (param_.l1 * (entry.z > 0 ? 1 : (entry.z < 0 ? -1 : 0)) - entry.z) /
                          (param_.l2 + ((param_.beta + sqrt(entry.n)) / param_.alpha));
            }
            entry.t = timestamp;
#ifdef USE_TBB
            it->second = entry;
#else
            weights_.insert(std::pair<uint64_t, FTRLEntry>(key, entry));
#endif
        }
        else
        {
            double sigma = (sqrt(it->second.n + g * g) - sqrt(it->second.n)) / param_.alpha;
            it->second.g = g;
            it->second.z += g - sigma * it->second.x;
            it->second.n += g * g;
            if (fabs(it->second.z) >= param_.l1)
            {
                it->second.x = (param_.l1 * (it->second.z > 0 ? 1 : (it->second.z < 0 ? -1 : 0)) - it->second.z) /
                               (param_.l2 + ((param_.beta + sqrt(it->second.n)) / param_.alpha));
            }
            else
            {
                it->second.x = 0.0;
            }
            it->second.t = timestamp;
        }
#ifdef USE_TBB
        it.release();
#endif
    }

    double xnorm() {
        double s = 0.0;
#ifdef USE_TBB
        tbb::concurrent_hash_map<uint64_t, FTRLEntry, UInt64Compare>::const_iterator it;
#else
        std::tr1::unordered_map<uint64_t, FTRLEntry>::iterator it;
#endif
        for (it = weights_.begin(); it != weights_.end(); ++it) {
            s += it->second.x * it->second.x;
        }
        return sqrt(s);
    }

    double gnorm() {
        double s = 0.0;
#ifdef USE_TBB
        tbb::concurrent_hash_map<uint64_t, FTRLEntry, UInt64Compare>::const_iterator it;
#else
        std::tr1::unordered_map<uint64_t, FTRLEntry>::iterator it;
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
        tbb::concurrent_hash_map<uint64_t, FTRLEntry, UInt64Compare>::const_iterator it;
#else
        std::tr1::unordered_map<uint64_t, FTRLEntry>::iterator it;
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
        tbb::concurrent_hash_map<uint64_t, FTRLEntry, UInt64Compare>::const_iterator it;
#else
        std::tr1::unordered_map<uint64_t, FTRLEntry>::iterator it;
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
        FTRLEntry entry;
#ifdef USE_TBB
        tbb::concurrent_hash_map<uint64_t, FTRLEntry, UInt64Compare>::accessor it;
#endif
        while (in >> index >> entry) {
#ifdef USE_TBB
            weights_.insert(it, index);
            it->second = entry;
            it.release();
#else
            weights_.insert(std::pair<uint64_t, FTRLEntry>(index, entry));;
#endif
        }
        in.close();

        return 0;
    }

    void reset() { weights_.clear(); }

    int remove_expired(time_t exp) {
#ifdef USE_TBB
        uint64_t key;
        tbb::concurrent_hash_map<uint64_t, FTRLEntry, UInt64Compare>::const_iterator it;
#else
        std::tr1::unordered_map<uint64_t, FTRLEntry>::iterator it;
#endif
        for (it = weights_.begin(); it != weights_.end(); ) {
            if (it->second.t < exp) { 
                llog_write(llog_, LL_DEBUG, "remove %llu %d", it->first, it->second.t);
#ifdef USE_TBB
                key = it->first;
                ++it;
                weights_.erase(key);
#else
                weights_.erase(it++);
#endif
            }
            else ++it;
        }
        return 0;
    }

private:
    FTRLParam param_;
#ifdef USE_TBB
    tbb::concurrent_hash_map<uint64_t, FTRLEntry, UInt64Compare> weights_;
#else
    std::tr1::unordered_map<uint64_t, FTRLEntry> weights_;
#endif

};

#endif
