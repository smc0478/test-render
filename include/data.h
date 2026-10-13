#ifndef _DATA_H_
#define _DATA_H_

namespace RD {

template<typename T>
class RData<T> {

private:
    T *data;
};

}

#endif // data.h