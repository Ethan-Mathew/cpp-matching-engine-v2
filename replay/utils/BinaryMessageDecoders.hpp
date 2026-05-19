#pragma once

#include "data/DecodedMessageTypes.hpp"

#include <fstream>

namespace lob::replay::utils
{

data::SystemEventMessage decode_s_type_message(std::ifstream& file);
data::StockDirectoryMessage decode_r_type_message(std::ifstream& file);
data::AddOrderMessage decode_a_type_message(std::ifstream& file);
data::AddOrderWithMPIDMessage decode_f_type_message(std::ifstream& file);
data::OrderExecutedMessage decode_e_type_message(std::ifstream& file);
data::OrderExecutedWithPriceMessage decode_c_type_message(std::ifstream& file);
data::OrderCancelMessage decode_x_type_message(std::ifstream& file);
data::OrderDeleteMessage decode_d_type_message(std::ifstream& file);
data::OrderReplaceMessage decode_u_type_message(std::ifstream& file);

} // namespace lob::replay::utils