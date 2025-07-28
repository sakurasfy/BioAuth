

#ifndef BIOAUTH_FAKEMULTIPLYGATE_H
#define BIOAUTH_FAKEMULTIPLYGATE_H


#include <memory>
#include <vector>
#include <array>
#include <algorithm>
#include <stdexcept>

#include "utils/linear_algebra.h"
#include "share/IsSpdz2kShare.h"
#include "fake-offline/FakeGate.h"


namespace bioauth {


template <IsSpdz2kShare ShrType, std::size_t N>
class FakeMultiplyGate : public FakeGate<ShrType, N> {
public:
    using SemiShrType = typename ShrType::SemiShrType;
    using ClearType = typename ShrType::ClearType;

    FakeMultiplyGate(const std::shared_ptr<FakeGate<ShrType, N>>& p_input_x,
                     const std::shared_ptr<FakeGate<ShrType, N>>& p_input_y);

    [[nodiscard]] auto dim_mid() const { return dim_mid_; }

protected:
    void doRunOffline() override;

private:
    std::size_t dim_mid_;
};


template <IsSpdz2kShare ShrType, std::size_t N>
FakeMultiplyGate<ShrType, N>::
FakeMultiplyGate(const std::shared_ptr<FakeGate<ShrType, N>>& p_input_x,
                 const std::shared_ptr<FakeGate<ShrType, N>>& p_input_y)
    : FakeGate<ShrType, N>(p_input_x, p_input_y), dim_mid_(p_input_x->dim_col()) {
    // Check and set dimensions
    if (p_input_x->dim_col() != p_input_y->dim_row()) {
        throw std::invalid_argument("The inputs of multiplication gate should have compatible dimensions");
    }

    this->set_dim_row(p_input_x->dim_row());
    this->set_dim_col(p_input_y->dim_col());
    // dim_mid_ was set in the initializer list
}


template <IsSpdz2kShare ShrType, std::size_t N>
void FakeMultiplyGate<ShrType, N>::doRunOffline() {
    auto size_lhs = this->dim_row() * this->dim_mid();
    auto size_rhs = this->dim_mid() * this->dim_col();
    auto size_output = this->dim_row() * this->dim_col();

    // $\lambda_z$ = rand()
    this->lambda_clear().resize(size_output);
    std::ranges::generate(this->lambda_clear(), getRand<ClearType>);

    auto lambda_shares_and_macs = this->fake_party().GenerateAllPartiesShares(this->lambda_clear());
    this->lambda_shr() = std::move(lambda_shares_and_macs.value_shares);
    this->lambda_shr_mac() = std::move(lambda_shares_and_macs.mac_shares);

    // Generate the multiplication triples
    std::vector<ClearType> a_clear(size_lhs);
    std::vector<ClearType> b_clear(size_rhs);
    std::ranges::generate(a_clear, getRand<ClearType>);
    std::ranges::generate(b_clear, getRand<ClearType>);
    auto c_clear = matrixMultiply(a_clear, b_clear, this->dim_row(), this->dim_mid(), this->dim_col());

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
    // 模拟 MAC check 通信（不验证，只统计）
    std::vector<SemiShrType> alpha_mul_lambda(this->lambda_shr()[0].size());
    for (size_t i = 0; i < alpha_mul_lambda.size(); ++i) {
        alpha_mul_lambda[i] = this->fake_party().mac_key(0) * this->lambda_shr()[0][i];
    }

    this->fake_party().SimulateSendToOther(alpha_mul_lambda);
    this->fake_party().SimulateSendToOther(this->lambda_shr_mac()[0]);
}


} // namespace bioauth
#endif //BIOAUTH_FAKEMULTIPLYGATE_H
