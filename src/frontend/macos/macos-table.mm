/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Volker Böhm
 * @copyright Copyright (c) 2025 Volker Böhm
 */

/**
 * @file macos-table.mm
 * @brief QaplaWindows::ImGuiTable shown in an NSTableView.
 *
 * The table view never reads the application's table: it reads a copy taken in the frame
 * whenever the table's version changed. AppKit asks a data source for rows whenever it likes --
 * while scrolling, between frames -- and by then the table may have changed or be gone.
 */

#import "macos-views.h"

#include "imgui-table.h"
#include "i18n.h"

#include <algorithm>
#include <cmath>

namespace {
    struct Cell {
        std::string text;
        bool alignRight = false;
    };
}

@interface QPTableSource : NSObject <NSTableViewDataSource, NSTableViewDelegate>
@property (nonatomic) QaplaMac::Widget* widget;
@property (nonatomic, strong) NSTableView* tableView;
@property (nonatomic, strong) NSScrollView* scrollView;
@end

@implementation QPTableSource {
@public
    std::vector<std::vector<Cell>> rows_;
    std::vector<size_t> rowIndex_;          ///< The table's row shown at each position
    std::vector<std::string> columnNames_;
    std::vector<float> setWidths_;          ///< Widths last given, so a user's resizing stays
    std::uint64_t version_;
    bool hasVersion_;
    bool clickable_;
    bool updatingSelection_;
    QaplaUi::Font font_;
    std::optional<std::pair<size_t, bool>> sort_;
    bool sortChanged_;
}

- (NSInteger)numberOfRowsInTableView:(NSTableView*)tableView {
    return static_cast<NSInteger>(rows_.size());
}

- (NSView*)tableView:(NSTableView*)tableView viewForTableColumn:(NSTableColumn*)column row:(NSInteger)row {
    NSTextField* field = [tableView makeViewWithIdentifier:@"cell" owner:self];
    if (field == nil) {
        field = [NSTextField labelWithString:@""];
        field.identifier = @"cell";
        field.lineBreakMode = NSLineBreakByClipping;
        field.cell.truncatesLastVisibleLine = NO;
    }
    const auto columnIndex = static_cast<size_t>([column.identifier integerValue]);
    field.font = QaplaMac::nsFont(font_, QaplaMac::baseFontSize(font_));
    const auto rowIndex = static_cast<size_t>(row);
    if (rowIndex < rows_.size() && columnIndex < rows_[rowIndex].size()) {
        const auto& cell = rows_[rowIndex][columnIndex];
        field.stringValue = [NSString stringWithUTF8String:cell.text.c_str()] ?: @"";
        field.alignment = cell.alignRight ? NSTextAlignmentRight : NSTextAlignmentLeft;
    } else {
        field.stringValue = @"";
    }
    return field;
}

- (BOOL)tableView:(NSTableView*)tableView shouldSelectRow:(NSInteger)row {
    return clickable_ || updatingSelection_;
}

- (void)tableViewSelectionDidChange:(NSNotification*)notification {
    if (updatingSelection_ || self.widget == nullptr) {
        return;
    }
    const NSInteger selected = self.tableView.selectedRow;
    if (selected >= 0 && static_cast<size_t>(selected) < rowIndex_.size()) {
        self.widget->pickedRow = rowIndex_[static_cast<size_t>(selected)];
        QaplaMac::scheduleFrame();
    }
}

- (void)tableView:(NSTableView*)tableView sortDescriptorsDidChange:(NSArray<NSSortDescriptor*>*)oldDescriptors {
    NSSortDescriptor* first = tableView.sortDescriptors.firstObject;
    if (first == nil) {
        sort_.reset();
    } else {
        sort_ = std::make_pair(static_cast<size_t>([first.key integerValue]), static_cast<bool>(first.ascending));
    }
    sortChanged_ = true;
    QaplaMac::scheduleFrame();
}

@end

namespace QaplaMac {

    NSView* makeTableView(Widget& widget) {
        QPTableSource* source = [[QPTableSource alloc] init];
        source.widget = &widget;

        NSTableView* tableView = [[NSTableView alloc] initWithFrame:NSZeroRect];
        tableView.dataSource = source;
        tableView.delegate = source;
        tableView.style = NSTableViewStylePlain;
        tableView.columnAutoresizingStyle = NSTableViewNoColumnAutoresizing;
        tableView.allowsColumnReordering = NO;
        tableView.allowsMultipleSelection = NO;
        tableView.allowsEmptySelection = YES;
        tableView.intercellSpacing = NSMakeSize(8, 2);
        tableView.gridStyleMask = NSTableViewGridNone;

        NSScrollView* scroll = [[NSScrollView alloc] initWithFrame:NSZeroRect];
        scroll.documentView = tableView;
        scroll.hasVerticalScroller = YES;
        scroll.hasHorizontalScroller = YES;
        scroll.autohidesScrollers = YES;
        scroll.borderType = NSNoBorder;
        scroll.drawsBackground = NO;
        tableView.backgroundColor = [NSColor clearColor];

        source.tableView = tableView;
        source.scrollView = scroll;
        widget.dataSource = source;
        return scroll;
    }

    void releaseTableView(Widget& widget) {
        QPTableSource* source = widget.dataSource;
        if (source != nil) {
            source.widget = nullptr;
            source.tableView.dataSource = nil;
            source.tableView.delegate = nil;
        }
        widget.dataSource = nil;
    }

    namespace {
        void updateColumns(QPTableSource* source, const QaplaWindows::ImGuiTable& table, float width) {
            NSTableView* tableView = source.tableView;
            const auto& columns = table.columns();
            const bool sortable = QaplaUi::hasFlag(table.tableFlags(), QaplaUi::TableFlags::Sortable);

            std::vector<std::string> names;
            names.reserve(columns.size());
            for (const auto& column : columns) {
                names.push_back(QaplaWindows::Translator::instance().translate("Table", column.name));
            }

            if (names != source->columnNames_ || tableView.tableColumns.count != columns.size()) {
                while (tableView.tableColumns.count > 0) {
                    [tableView removeTableColumn:tableView.tableColumns.lastObject];
                }
                for (size_t index = 0; index < columns.size(); ++index) {
                    NSString* identifier = [NSString stringWithFormat:@"%zu", index];
                    NSTableColumn* column = [[NSTableColumn alloc] initWithIdentifier:identifier];
                    column.title = [NSString stringWithUTF8String:names[index].c_str()] ?: @"";
                    column.headerCell.alignment = columns[index].alignRight ? NSTextAlignmentRight
                                                                            : NSTextAlignmentLeft;
                    column.resizingMask = NSTableColumnUserResizingMask;
                    if (sortable) {
                        column.sortDescriptorPrototype = [NSSortDescriptor sortDescriptorWithKey:identifier
                                                                                       ascending:YES];
                    }
                    [tableView addTableColumn:column];
                }
                source->columnNames_ = names;
                source->setWidths_.clear();
            }

            // Fixed columns keep the width the application gave them, measured in the table's own
            // font; a column without one takes what is left, or the width of its longest cell.
            float used = 0.0F;
            std::vector<float> widths(columns.size(), 0.0F);
            for (size_t index = 0; index < columns.size(); ++index) {
                widths[index] = table.layoutWidth(index);
                used += widths[index] + 8.0F;
            }
            for (size_t index = 0; index < columns.size(); ++index) {
                if (widths[index] <= 0.0F) {
                    const float rest = width - used - 16.0F;
                    widths[index] = std::max(120.0F, rest);
                    used += widths[index];
                }
                NSTableColumn* column = tableView.tableColumns[index];
                if (index >= source->setWidths_.size() || std::abs(source->setWidths_[index] - widths[index]) > 0.5F) {
                    column.width = widths[index];
                }
            }
            source->setWidths_ = widths;
        }

        void takeSnapshot(QPTableSource* source, const QaplaWindows::ImGuiTable& table) {
            const size_t count = table.visibleRowCount();
            const size_t columns = table.columns().size();
            source->rows_.assign(count, {});
            source->rowIndex_.assign(count, 0);
            for (size_t position = 0; position < count; ++position) {
                const size_t row = table.rowAt(position);
                source->rowIndex_[position] = row;
                auto& cells = source->rows_[position];
                const size_t cellCount = std::min(columns, table.cellCount(row));
                cells.resize(cellCount);
                for (size_t column = 0; column < cellCount; ++column) {
                    bool right = false;
                    cells[column].text = table.cellText(row, column, right);
                    cells[column].alignRight = right;
                }
            }
        }
    }

    void updateTableView(Widget& widget, QaplaWindows::ImGuiTable& table, NSRect frame) {
        QPTableSource* source = widget.dataSource;
        if (source == nil) {
            return;
        }
        NSTableView* tableView = source.tableView;

        source->clickable_ = table.isClickable();
        source->font_ = table.font();
        tableView.usesAlternatingRowBackgroundColors =
            QaplaUi::hasFlag(table.tableFlags(), QaplaUi::TableFlags::RowBg);
        const CGFloat rowHeight = QaplaMac::lineHeight(source->font_, QaplaMac::baseFontSize(source->font_)) + 2.0;
        if (std::abs(tableView.rowHeight - rowHeight) > 0.5) {
            tableView.rowHeight = rowHeight;
        }

        updateColumns(source, table, static_cast<float>(frame.size.width));

        // The order is the application's to keep: a header click is handed to the table, which
        // sorts its index, and the copy below shows the result.
        if (QaplaUi::hasFlag(table.tableFlags(), QaplaUi::TableFlags::Sortable) &&
            (table.needsSort() || source->sortChanged_)) {
            if (source->sort_) {
                table.sortBy(source->sort_->first, source->sort_->second);
            } else {
                table.sortBy(std::nullopt, true);
            }
            source->sortChanged_ = false;
        }

        if (!source->hasVersion_ || source->version_ != table.version()) {
            takeSnapshot(source, table);
            source->version_ = table.version();
            source->hasVersion_ = true;
            [tableView reloadData];
        }

        // The current row as the application has it, shown as the selection.
        source->updatingSelection_ = true;
        auto current = table.currentRow();
        auto position = current ? table.positionOf(*current) : std::nullopt;
        if (position && table.isClickable()) {
            if (tableView.selectedRow != static_cast<NSInteger>(*position)) {
                [tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:*position] byExtendingSelection:NO];
            }
        } else if (tableView.selectedRow >= 0) {
            [tableView deselectAll:nil];
        }
        source->updatingSelection_ = false;

        if (auto scrollTo = table.takeScrollToRow()) {
            if (auto target = table.positionOf(*scrollTo)) {
                [tableView scrollRowToVisible:static_cast<NSInteger>(*target)];
            }
        }
    }

} // namespace QaplaMac
