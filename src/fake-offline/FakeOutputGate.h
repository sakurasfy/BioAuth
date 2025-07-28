

#ifndef BIOAUTH_FAKEOUTPUTGATE_H
#define BIOAUTH_FAKEOUTPUTGATE_H

#include <algorithm>

#include "utils/rand.h"
#include "share/IsSpdz2kShare.h"
#include "fake-offline/FakeGate.h"


namespace bioauth {

template <IsSpdz2kShare ShrType, std::size_t N>
class FakeOutputGate : public FakeGate<ShrType, N> {
public:
    using ClearType = typename ShrType::ClearType;
    using SemiShrType = typename ShrType::SemiShrType;

    explicit FakeOutputGate(const std::shared_ptr<FakeGate<ShrType, N>>& p_input_x);

private:
    void doRunOffline() override;
};


template <IsSpdz2kShare ShrType, std::size_t N>
FakeOutputGate<ShrType, N>::FakeOutputGate(const std::shared_ptr<FakeGate<ShrType, N>>& p_input_x)
    : FakeGate<ShrType, N>(p_input_x, nullptr) {
    this->set_dim_row(p_input_x->dim_row());
    this->set_dim_col(p_input_x->dim_col());
}

template <IsSpdz2kShare ShrType, std::size_t N>
void FakeOutputGate<ShrType, N>::doRunOffline() {}  // Do nothing

} // bioauth

#endif //BIOAUTH_FAKEOUTPUTGATE_H
