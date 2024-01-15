#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <map>
#include <set>

class Sheet : public SheetInterface
{
public:
    ~Sheet();

    void ClearCell(Position pos) override;
    void SetCell(Position pos, std::string text) override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;
    Size GetPrintableSize() const override;

    void InvalidateCell(const Position& pos);
    const std::set<Position> GetDependentCells(const Position& pos);
    void DeleteDependencies(const Position& pos);
    void AddDependentCell(const Position& main_cell, const Position& dependent_cell);

private:
    std::map<Position, std::set<Position>> cells_dependencies_;
    std::vector<std::vector<std::unique_ptr<Cell>>> sheet_;

    bool area_is_valid_ = true;
    int max_row_ = 0;
    int max_col_ = 0;

    void UpdatePrintableSize();
    bool CellExists(Position pos) const;
    void Touch(Position pos);
    void ControlPosition(Position pos) const;
};