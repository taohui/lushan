#ifndef PS_H
#define PS_H
#include "llog.h"
#include <pthread.h>
#include <string>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <tr1/unordered_map>
#include <fstream>
#include <sstream>
#include <map>

/*  Authors: */
/*      Tao Hui <taohui3@gmail.com> */

#define STATE_FILE_NAME "state.txt"

struct KVPair {
    uint64_t key;
    double value;
};

enum ModelType {
    LR = 1,
    FM
};

class ParameterServer {
public:
    ParameterServer(llog_t *llog, const char *home_path) : llog_(llog), 
        home_path_(home_path), model_type_(LR) {
        pthread_mutex_init(&mutex_, NULL);
    }

    virtual ~ParameterServer() {
        pthread_mutex_lock(&mutex_);
        llog_close(llog_);
        pthread_mutex_unlock(&mutex_);
    }

    virtual double get(uint64_t key) = 0;
    virtual void update(uint64_t key, double g, time_t timestamp) = 0;
    virtual double xnorm() = 0;
    virtual double gnorm() = 0;
    virtual int write_weights(const char *filename) = 0;
    virtual int write_state(const char *filename) = 0;
    virtual int read_state(const char *filename) = 0;
    virtual void reset() = 0;
    virtual int remove_expired(time_t exp) { return 0; };

    llog_t *get_llog() { return llog_; }

    int lock() {
        return pthread_mutex_lock(&mutex_);
    }
    int unlock() {
        return pthread_mutex_unlock(&mutex_);
    }

    void set_model_type(ModelType model_type) { model_type_ = model_type; }

    int remove_file(const char *filename) {
        char filepath[1024];
        snprintf(filepath, 1024, "%s/%s", home_path_.c_str(), filename);
        return unlink(filepath);
    }

protected:
    llog_t *llog_;
    pthread_mutex_t mutex_;
    std::string home_path_;
    ModelType model_type_;
};

#endif
