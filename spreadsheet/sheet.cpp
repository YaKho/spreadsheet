#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    
    if (!cells_.count(pos)) {
        cells_.emplace(pos, std::make_unique<Cell>(*this));
    }        
    cells_.at(pos)->Set(std::move(text));
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    if (cells_.count(pos)) {
        return cells_.at(pos).get();
    }
    return nullptr;
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    if (cells_.count(pos)) {
        return cells_.at(pos).get();
    }
    return nullptr;
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }

    if (cells_.count(pos) && cells_.at(pos).get() != nullptr) {
        cells_.at(pos)->Clear();
        cells_.at(pos).reset();
    }
}

Size Sheet::GetPrintableSize() const {
    Size res{ 0, 0 };

    for (const auto& [pos, ptr] : cells_) {
        if (ptr.get() != nullptr) {
            res.rows = std::max(res.rows, pos.row + 1);
            res.cols = std::max(res.cols, pos.col + 1);
        }
    }
    return res;
}

void Sheet::PrintValues(std::ostream& output) const {
    auto size = GetPrintableSize();
    for (int i = 0; i < size.rows; ++i) {
        for (int j = 0; j < size.cols; ++j) {
            if (j > 0) {
                output << "\t";
            }
            if (cells_.count({ i, j })) {
                auto cell = cells_.at(Position{ i, j }).get();
                if (cell != nullptr && !cell->GetText().empty()) {
                    std::visit([&](const auto arg) { output << arg; }, cell->GetValue());
                }
            }
        }
        output << "\n";
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    auto size = GetPrintableSize();
    for (int i = 0; i < size.rows; ++i) {
        for (int j = 0; j < size.cols; ++j) {
            if (j > 0) {
                output << "\t";
            }
            if (cells_.count({ i, j })) {
                auto cell = cells_.at(Position{ i, j }).get();
                if (cell != nullptr && !cell->GetText().empty()) {
                    output << cell->GetText();
                }
            }
        }
        output << "\n";
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
