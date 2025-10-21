#include "ir/graph.h"
#include "ir/basic_block.h"
#include "ir/instruction.h"
#include <ostream>

BasicBlock* Graph::CreateBasicBlock() {
    blocks_.emplace_back(next_block_id_++, this);
    if (start_block_ == nullptr) {
        start_block_ = &blocks_.back();
    }
    return &blocks_.back();
}

// TODO: this method does not belong to the Graph class, it should be in the Instruction class
User* Graph::RegisterUse(Instruction* def, Instruction* user_inst, uint32_t input_idx) {
    auto user = std::make_unique<User>(user_inst, input_idx);
    User* node = user.get();
    node->SetNextUser(def->head_user_);
    def->head_user_ = node;
    users_.push_back(std::move(user));
    return node;
}


// void Graph::Dump(std::ostream& os) const {
//     for (const auto& bb : blocks_) {
//         bb.Dump(os);
//     }
// }


void Graph::Dump(std::ostream& os) const {

    os << "Function Arguments:\n";
    if (args_.empty()) {
        os << "  (none)\n";
    } else {
        for (const auto* arg : args_) {
            os << "  ";
            arg->Print(os);
            os << '\n';
        }
    }
    os << "\n";

    for (const auto& bb : blocks_) {
        bb.Dump(os);
        os << '\n';
    }
}


