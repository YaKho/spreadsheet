#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <unordered_map>

struct PositionHasher {
    std::hash<int> hasher;

    size_t operator()(const Position pos) const {
        return hasher(pos.row) * 37 + hasher(pos.col);
    }
};

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    std::unordered_map<Position, 
                        std::unique_ptr<Cell>, 
                        PositionHasher> cells_;
};