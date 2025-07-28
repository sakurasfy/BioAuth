
#ifndef BIOAUTH_FAKECONV2DGATE_H
#define BIOAUTH_FAKECONV2DGATE_H

#include <memory>
#include <vector>
#include <array>
#include <algorithm>
#include <stdexcept>

#include "utils/linear_algebra.h"
#include "utils/tensor.h"
#include "share/IsSpdz2kShare.h"
#include "fake-offline/FakeGate.h"


namespace bioauth {


template <IsSpdz2kShare ShrType, std::size_t N>
class FakeConv2DGate : public FakeGate<ShrType, N> {
public:
    using SemiShrType = typename ShrType::SemiShrType;
    using ClearType = typename ShrType::ClearType;

    /// @brief
    /// @param p_input_x: The input tensor
    /// @param p_input_y: The kernel tensor
    FakeConv2DGate(const std::shared_ptr<FakeGate<ShrType, N>>& p_input_x,
                   const std::shared_ptr<FakeGate<ShrType, N>>& p_input_y,
                   const Conv2DOp& op);

protected:
    void doRunOffline() override;

private:
    Conv2DOp conv_op_;
};


template <IsSpdz2kShare ShrType, std::size_t N>
FakeConv2DGate<ShrType, N>::
FakeConv2DGate(const std::shared_ptr<FakeGate<ShrType, N>>& p_input_x,
               const std::shared_ptr<FakeGate<ShrType, N>>& p_input_y,
               const Conv2DOp& op)
    : FakeGate<ShrType, N>(p_input_x, p_input_y), conv_op_(op) {
    this->set_dim_row(conv_op_.compute_output_size());
    this->set_dim_col(1);
}


template <IsSpdz2kShare ShrType, std::size_t N>
void FakeConv2DGate<ShrType, N>::doRunOffline() {
    auto size_lhs = conv_op_.compute_input_size();
    auto size_rhs = conv_op_.compute_kernel_size();
    auto size_output = conv_op_.compute_output_size();

    // $\lambda_z$ = rand()
    this->lambda_clear().resize(size_output);
    std::ranges::generate(this->lambda_clear(), getRand<ClearType>);

    auto lambda_share_and_macs = this->fake_party().GenerateAllPartiesShares(this->lambda_clear());
    this->lambda_shr() = std::move(lambda_share_and_macs.value_shares);
    this->lambda_shr_mac() = std::move(lambda_share_and_macs.mac_shares);

    // Generate the convolution triples
    std::vector<ClearType> a_clear(size_lhs);
    std::vector<ClearType> b_clear(size_rhs);
    std::ranges::generate(a_clear, getRand<ClearType>);
    std::ranges::generate(b_clear, getRand<ClearType>);
    auto c_clear = convolution(a_clear, b_clear, conv_op_);

    auto a_share_with_mac = this->fake_party().GenerateAllPartiesShares(a_clear);
    auto b_share_with_mac = this->fake_party().GenerateAllPartiesShares(b_clear);
    auto c_share_with_mac = this->fake_party().GenerateAllPartiesShares(c_clear);

    // $\delta_x = a - \lambda_x$, $\delta_y = b - \lambda_y$
    auto delta_x_clear = matrixSubtract(a_clear, this->input_x()->lambda_clear());
    auto delta_y_clear = matrixSubtract(b_clear, this->input_y()->lambda_clear());

    // Wrtie all data to files
    this->fake_party().WriteSharesToAllParites(a_share_with_mac.value_shares);
    this->fake_party().WriteSharesToAllParites(a_share_with_mac.mac_shares);
    this->fake_party().WriteSharesToAllParites(b_share_with_mac.value_shares);
    this->fake_party().WriteSharesToAllParites(b_share_with_mac.mac_shares);
    this->fake_party().WriteSharesToAllParites(c_share_with_mac.value_shares);
    this->fake_party().WriteSharesToAllParites(c_share_with_mac.mac_shares);
    this->fake_party().WriteSharesToAllParites(this->lambda_shr());
    this->fake_party().WriteSharesToAllParites(this->lambda_shr_mac());

    this->fake_party().WriteClearToAllParties(delta_x_clear);
    this->fake_party().WriteClearToAllParties(delta_y_clear);
}

} // namespace bioauth


#endif //BIOAUTH_FAKECONV2DGATE_H
