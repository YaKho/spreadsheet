#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <stack>


class Cell::Impl {
public:
	virtual ~Impl() = default;
	virtual Value GetValue(const SheetInterface& sheet) const = 0;
	virtual std::string GetText() const = 0;
	virtual std::vector<Position> GetReferencedCells() const { 
		return {}; 
	}
	virtual void ClearCache() const {}
};

class Cell::EmptyImpl : public Impl {
public:
	Value GetValue([[maybe_unused]] const SheetInterface& sheet) const override {
		return "";
	}

	std::string GetText() const override {
		return "";
	}
};

class Cell::TextImpl : public Impl {
public:
	TextImpl(std::string text) : text_(std::move(text)) {
		if (text_.empty()) {
			throw std::logic_error("");
		}
	}

	Value GetValue([[maybe_unused]] const SheetInterface& sheet) const override {
		if (text_[0] == ESCAPE_SIGN) {
			return text_.substr(1);
		}
		return text_;
	}

	std::string GetText() const override {
		return text_;
	}

private:
	std::string text_;
};

class Cell::FormulaImpl : public Impl {
public:
	explicit FormulaImpl(std::string expr) {
		if (expr.empty() || expr[0] != FORMULA_SIGN) {
			throw std::logic_error("");
		}
		formula_ = ParseFormula(expr.substr(1));
	}

	Value GetValue(const SheetInterface& sheet) const override {
		if (!cache_) {
			cache_ = formula_->Evaluate(sheet);
		}
		if (std::holds_alternative<double>(cache_.value())) {
			return std::get<double>(cache_.value());
		}
		return std::get<FormulaError>(cache_.value());
	}

	std::string GetText() const override {
		return FORMULA_SIGN + formula_->GetExpression();
	}

	std::vector<Position> GetReferencedCells() const override {
		return formula_->GetReferencedCells();
	}

	void ClearCache() const override {
		cache_.reset();
	}

private:
	std::unique_ptr<FormulaInterface> formula_;
	mutable std::optional<FormulaInterface::Value> cache_;
};


Cell::Cell(Sheet& sheet) 
	: impl_(std::make_unique<EmptyImpl>())
	, sheet_(sheet)	{}

Cell::~Cell() {}

void Cell::Set(std::string text) {
	std::unique_ptr<Impl> tmp;
		
	if (text.empty()) {
		tmp = std::make_unique<EmptyImpl>();
	} else if (text.size() > 1 && text[0] == FORMULA_SIGN) {
		tmp = std::make_unique<FormulaImpl>(std::move(text));
		if (CheckCycles(*tmp)) {
			throw CircularDependencyException("Cycle found!");
		}
	} else {
		tmp = std::make_unique<TextImpl>(std::move(text));
	}
	impl_ = std::move(tmp);

	for (auto node : parents_nodes_) {
		node->children_nodes_.erase(this);
	}
	parents_nodes_.clear();

	for (const auto& pos : GetReferencedCells()) {
		auto parent = dynamic_cast<Cell*>(sheet_.GetCell(pos));
		if (parent == nullptr) {
			sheet_.SetCell(pos, "");
			parent = dynamic_cast<Cell*>(sheet_.GetCell(pos));
		}
		parent->children_nodes_.insert(this);
		parents_nodes_.insert(parent);
	}
	ClearCacheRec();
}

void Cell::Clear() {
	impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const {
	return impl_->GetValue(sheet_);
}

std::string Cell::GetText() const {
	return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
	return impl_->GetReferencedCells();
}

bool Cell::IsReferenced() const {
	return !children_nodes_.empty();
}

void Cell::ClearCache() {
	impl_->ClearCache();
}

void Cell::ClearCacheRec() {
	ClearCache();
	for (auto node : children_nodes_) {
		node->ClearCacheRec();
	}
}

bool Cell::CheckCycles(const Impl& impl) const {
	std::unordered_set<const Cell*> visited_cells;

	std::stack<const Cell*> st;

	for (const auto& pos : impl.GetReferencedCells()) {
		st.push(dynamic_cast<const Cell*>(sheet_.GetCell(pos)));
	}
	
	while (!st.empty()) {
		const Cell* cur = st.top();
		st.pop();

		if (cur == nullptr) {
			continue;
		}

		visited_cells.insert(cur);

		if (cur == this) {
			return true;
		}

		for (const auto& pos : cur->GetReferencedCells()) {
			auto cell = dynamic_cast<const Cell*>(sheet_.GetCell(pos));
			if (!visited_cells.count(cell)) {
				st.push(cell);
			}
		}
	}

	return false;
}