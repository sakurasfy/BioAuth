

#ifndef BIOAUTH_MULTIPLYGATE_H
#define BIOAUTH_MULTIPLYGATE_H

#include <memory>
#include <vector>
#include <stdexcept>
#include <thread>
#include <numeric>

#include "protocols/Gate.h"
#include "share/IsSpdz2kShare.h"
#include "utils/linear_algebra.h"

namespace bioauth {

template <IsSpdz2kShare ShrType>
class MultiplyGate : public Gate<ShrType> {
public:
    using SemiShrType = typename ShrType::SemiShrType;

    MultiplyGate(const std::shared_ptr<Gate<ShrType>>& p_input_x,
                 const std::shared_ptr<Gate<ShrType>>& p_input_y);

    [[nodiscard]] std::size_t dim_mid() const { return dim_mid_; }

protected:
    void doReadOfflineFromFile() override;
    void doRunOnline() override;

private:
    std::size_t dim_mid_;

    std::vector<SemiShrType> a_shr_, a_shr_mac_;
    std::vector<SemiShrType> b_shr_, b_shr_mac_;
    std::vector<SemiShrType> c_shr_, c_shr_mac_;
    std::vector<SemiShrType> delta_x_clear_;
    std::vector<SemiShrType> delta_y_clear_;
};

template <IsSpdz2kShare ShrType>
MultiplyGate<ShrType>::
MultiplyGate(const std::shared_ptr<Gate<ShrType>>& p_input_x,
             const std::shared_ptr<Gate<ShrType>>& p_input_y)
    : Gate<ShrType>(p_input_x, p_input_y), dim_mid_(p_input_x->dim_col()) {
    // check and set dimensions
    if (p_input_x->dim_col() != p_input_y->dim_row()) {
        throw std::invalid_argument("The inputs of multiplication gate should have compatible dimensions");
    }
    this->set_dim_row(p_input_x->dim_row());
    this->set_dim_col(p_input_y->dim_col());
    // dim_mid_ was set in the initializer list
}

template <IsSpdz2kShare ShrType>
void MultiplyGate<ShrType>::doReadOfflineFromFile() {
    auto size_lhs = this->dim_row() * this->dim_mid();
    auto size_rhs = this->dim_mid() * this->dim_col();
    auto size_output = this->dim_row() * this->dim_col();

    a_shr_ = this->party().ReadShares(size_lhs);
    a_shr_mac_ = this->party().ReadShares(size_lhs);
    b_shr_ = this->party().ReadShares(size_rhs);
    b_shr_mac_ = this->party().ReadShares(size_rhs);
    c_shr_ = this->party().ReadShares(size_output);
    c_shr_mac_ = this->party().ReadShares(size_output);
    this->lambda_shr() = this->party().ReadShares(size_output);
    this->lambda_shr_mac() = this->party().ReadShares(size_output);
    delta_x_clear_ = this->party().ReadShares(size_lhs);
    delta_y_clear_ = this->party().ReadShares(size_rhs);
}

template <IsSpdz2kShare ShrType>
void MultiplyGate<ShrType>::doRunOnline() {
    // temp_x = $\Delta_x + \delta_x$
    auto temp_x = matrixAdd(this->input_x()->Delta_clear(), delta_x_clear_);
    // temp_y = $\Delta_y + \delta_y$
    //std::cout << "1: " << this->party().bytes_sent() << " bytes\n";
    auto temp_y = matrixAdd(this->input_y()->Delta_clear(), delta_y_clear_);
    // temp_xy = temp_x * temp_y
    //std::cout << "2: " << this->party().bytes_sent() << " bytes\n";
    auto temp_xy = matrixMultiply(temp_x, temp_y, this->dim_row(), this->dim_mid(), this->dim_col());
    //std::cout << "3: " << this->party().bytes_sent() << " bytes\n";
    
    // Compute [Delta_z] according to the paper
    // [Delta_z] = [c] + [lambda_z]
    auto Delta_z_shr = matrixAdd(c_shr_, this->lambda_shr());
    // [Delta_z] -= [a] * temp_y
    matrixSubtractAssign(Delta_z_shr,
                         matrixMultiply(a_shr_, temp_y, this->dim_row(), this->dim_mid(), this->dim_col()));
    // [Delta_z] -= temp_x * [b]
    matrixSubtractAssign(Delta_z_shr,
                         matrixMultiply(temp_x, b_shr_, this->dim_row(), this->dim_mid(), this->dim_col()));
    if (this->my_id() == 0) {
        // [Delta_z] += temp_xy
        matrixAddAssign(Delta_z_shr, temp_xy);
    }
    
    // Compute Delta_z_mac according to the paper结果验证
    // [Delta_z_mac] = temp_xy * [key]
    auto Delta_z_mac = std::move(temp_xy);
    matrixScalarAssign(Delta_z_mac, this->party().global_key_shr());
    // [Delta_z_mac] += [c_mac] + [lambda_z_mac]
    matrixAddAssign(Delta_z_mac,
                    matrixAdd(c_shr_mac_, this->lambda_shr_mac()));
    // [Delta_z_mac] -= [a_mac] * temp_y
    matrixSubtractAssign(Delta_z_mac,
                         matrixMultiply(a_shr_mac_, temp_y, this->dim_row(), this->dim_mid(), this->dim_col()));
    // [Delta_z_mac] -= temp_x * [b_mac]
    matrixSubtractAssign(Delta_z_mac,
                         matrixMultiply(temp_x, b_shr_mac_, this->dim_row(), this->dim_mid(), this->dim_col()));

    using ClearType = typename ShrType::ClearType;
    //ClearType inner_product_share = std::accumulate(Delta_z_shr.begin(), Delta_z_shr.end(), ClearType(0));
    auto inner_product_shares = Delta_z_shr;  // 直接使用每个 [Δ_z]

    //std::cout << "Before communication: " << this->party().bytes_sent() << " bytes\n";
    uint64_t before_ = this->party().bytes_sent();
    //std::cout << "Before communication: " << before_ << " bytes\n";
    //std::cout << "size: " << inner_product_shares.size() << " \n";
    std::thread t1([&, this]() {
        //this->party().SendVecToOther(std::vector<ClearType>{inner_product_share});  
        this->party().SendVecToOther(inner_product_shares);
    });

    //std::vector<ClearType> received_vec = this->party().template ReceiveVecFromOther<ClearType>(1);
    std::vector<ClearType> received_vec = this->party().template ReceiveVecFromOther<ClearType>(inner_product_shares.size());
    t1.join();
    //uint64_t after_ = this->party().bytes_sent();
    this->party().comm_actual_=this->party().bytes_sent()-before_;
    //this->party().comm_after_ = this->party().bytes_sent();
    //uint64_t bytes_sent_=bytes_sent_actual(comm_before,comm_after);

   // std::cout << "After communication: " << this->party().comm_actual_<< " bytes\n";
    //ClearType final_share = inner_product_share + received_vec[0];
    std::vector<ClearType> added_clear(inner_product_shares.size());
    for (std::size_t i = 0; i < inner_product_shares.size(); ++i) {
        added_clear[i] = inner_product_shares[i] + received_vec[i];
    }


    //this->Delta_clear().resize(1);
    //this->Delta_clear()[0] = final_share;


    // Since Delta_clear is in ClearType but stored in SemiShrType, we need to remove the upper bits
    // This is important since it affects the correctness in MultiplyTruncGate!!!
    //ShrType::RemoveUpperBitsInplace(this->Delta_clear());

    std::vector<SemiShrType> final_shares;
    final_shares.reserve(added_clear.size());
    for (const auto& val : added_clear) {
         final_shares.emplace_back(static_cast<SemiShrType>(val));  // 转换 ClearType → SemiShrType
    }
    this->Delta_clear() = std::move(final_shares);
    ShrType::RemoveUpperBitsInplace(this->Delta_clear());

    // free the spaces of preprocessing data
    a_shr_.clear();
    a_shr_.shrink_to_fit();
    a_shr_mac_.clear();
    a_shr_mac_.shrink_to_fit();
    b_shr_.clear();
    b_shr_.shrink_to_fit();
    b_shr_mac_.clear();
    b_shr_mac_.shrink_to_fit();
    c_shr_.clear();
    c_shr_.shrink_to_fit();
    c_shr_mac_.clear();
    c_shr_mac_.shrink_to_fit();
    delta_x_clear_.clear();
    delta_x_clear_.shrink_to_fit();
    delta_y_clear_.clear();
    delta_y_clear_.shrink_to_fit();
}

} // bioauth

#endif //BIOAUTH_MULTIPLYGATE_H
