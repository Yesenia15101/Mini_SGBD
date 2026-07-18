#include "../include/NestedLoopJoin.h"

NestedLoopJoin::NestedLoopJoin(Iterator* left_it,
                                std::function<Iterator*()> right_it_factory,
                                std::function<bool(const Row&, const Row&)> pred)
    : left(left_it), right_factory(right_it_factory), predicate(pred),
      left_valid(false), current_right(nullptr) {}

void NestedLoopJoin::open_right_for_current_left(){
    current_right = right_factory();
    current_right->open();
}

void NestedLoopJoin::open(){
    left->open();
    left_valid = left->next(current_left);

    if(left_valid)
        open_right_for_current_left();
}

bool NestedLoopJoin::next(Row& out){
    Row right_row;

    while(left_valid){
        while(current_right->next(right_row)){
            if(predicate(current_left, right_row)){
                out.key = current_left.key;
                out.fields = current_left.fields;
                out.fields.push_back(std::to_string(right_row.key));
                for(const auto& f : right_row.fields)
                    out.fields.push_back(f);
                return true;
            }
        }

        // se agoto el lado derecho para esta fila izquierda: pasar a la siguiente
        current_right->close();
        delete current_right;
        current_right = nullptr;

        left_valid = left->next(current_left);
        if(left_valid)
            open_right_for_current_left();
    }

    return false;
}

void NestedLoopJoin::close(){
    if(current_right != nullptr){
        current_right->close();
        delete current_right;
        current_right = nullptr;
    }
    left->close();
}