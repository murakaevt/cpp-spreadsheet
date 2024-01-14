#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <cmath>

Cell::Cell(SheetInterface& sheet)
    : sheet_(sheet)
{}

Cell::~Cell()
{
    if (impl_)
    {
        impl_.reset(nullptr);
    }
}

void Cell::Set(const std::string& text)
{
    using namespace std::literals; // Потому что в Set задается формула или текст, можно конечно реализовать в методе, который направлен на создания ячейки, но я отталкивался от этого
    // Лично я не особо вижу ошибку создаваить тут проверку
    if (text.empty())
    {
        impl_ = std::make_unique<EmptyImpl>();
        return;
    }
    if (text[0] != FORMULA_SIGN || (text[0] == FORMULA_SIGN && text.size() == 1))
    {
        impl_ = std::make_unique<TextImpl>(text);
        return;
    }
    try
    {
        impl_ = std::make_unique<FormulaImpl>(sheet_, std::string{ text.begin() + 1, text.end() });
    }
    catch (...)
    {
        std::string fe_msg = "Formula parsing error"s;
        throw FormulaException(fe_msg);
    }
}

void Cell::Clear()
{
    Set(nullptr);
}

Cell::Value Cell::GetValue() const
{
    return impl_->IGetValue();
}

std::string Cell::GetText() const
{
    return impl_->IGetText();
}

std::vector<Position> Cell::GetReferencedCells() const
{
    return impl_.get()->IGetReferencedCells();
}

bool Cell::IsCyclicDependent(const Cell* start_cell_ptr, const Position& end_pos) const {
    for (const auto &referenced_cell_pos: GetReferencedCells()) {
        if (referenced_cell_pos == end_pos) {
            return true;
        }
        const Cell *ref_cell_ptr = dynamic_cast<const Cell *>(sheet_.GetCell(referenced_cell_pos));

        if (!ref_cell_ptr) {
            sheet_.SetCell(referenced_cell_pos, "");
            ref_cell_ptr = dynamic_cast<const Cell *>(sheet_.GetCell(referenced_cell_pos));
        }
        if (ref_cell_ptr->IsCyclicDependent(start_cell_ptr, end_pos)) {
            return true;
        }
        if (start_cell_ptr == ref_cell_ptr) {
            return true;
        }
    }

    return false;
}
bool Cell::IsCacheValid() const
{
    return impl_->ICached();
}
void Cell::InvalidateCache()
{
    impl_->IInvalidateCache();
}
CellType Cell::EmptyImpl::IGetType() const
{
    return CellType::EMPTY;
}
std::string Cell::EmptyImpl::IGetText() const
{
    using namespace std::literals;

    return ""s;
}
CellInterface::Value Cell::EmptyImpl::IGetValue() const
{
    return 0.0;
}
std::vector<Position> Cell::EmptyImpl::IGetReferencedCells() const
{
    return {};
}
bool Cell::EmptyImpl::ICached() const
{
    return true;
}
void Cell::EmptyImpl::IInvalidateCache()
{
    return;
}


Cell::TextImpl::TextImpl(std::string text)
    : cell_text_(std::move(text))
{
    if (cell_text_[0] == ESCAPE_SIGN)
    {
        escaped_ = true;
    }
}

CellType Cell::TextImpl::IGetType() const
{
    return CellType::TEXT;
}

CellInterface::Value Cell::TextImpl::IGetValue() const
{
    if (escaped_)
    {
        return cell_text_.substr(1, cell_text_.size() - 1);
    }
    else
    {
        return cell_text_;
    }
}

std::vector<Position> Cell::TextImpl::IGetReferencedCells() const
{
    return {};
}
void Cell::TextImpl::IInvalidateCache()
{
    return;
}
std::string Cell::TextImpl::IGetText() const
{
    return cell_text_;
}
bool Cell::TextImpl::ICached() const
{
    return true;
}
CellType Cell::FormulaImpl::IGetType() const
{
    return CellType::FORMULA;
}
Cell::FormulaImpl::FormulaImpl(SheetInterface& sheet, std::string formula)
    : sheet_(sheet), formula_(ParseFormula(formula))
{}

CellInterface::Value Cell::FormulaImpl::IGetValue() const
{
    if (!cached_value_)
    {
        FormulaInterface::Value result = formula_->Evaluate(sheet_);
        if (std::holds_alternative<double>(result))
        {
            if (std::isfinite(std::get<double>(result)))
            {
                return std::get<double>(result);
            }
            else
            {
                return FormulaError(FormulaError::Category::Arithm);
            }
        }
        return std::get<FormulaError>(result);
    }
    return *cached_value_;
}

std::vector<Position> Cell::FormulaImpl::IGetReferencedCells() const
{
    return formula_.get()->GetReferencedCells();
}

std::string Cell::FormulaImpl::IGetText() const
{
    return { FORMULA_SIGN + formula_->GetExpression() };
}

bool Cell::FormulaImpl::ICached() const
{
    return cached_value_.has_value();
}

void Cell::FormulaImpl::IInvalidateCache()
{
    cached_value_.reset();
}