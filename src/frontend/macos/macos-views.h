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

#pragma once

/**
 * @file macos-views.h
 * @brief The views of the macOS frontend that AppKit has no ready-made equivalent for, and the
 * object that hands AppKit's actions and notifications back to the widgets.
 */

#import "macos-backend.h"

/**
 * @brief Receives the actions and delegate calls of one widget's controls.
 *
 * Holds a plain pointer to the widget; the widget clears it when it goes, so a late callback
 * finds nothing to write to.
 */
@interface QPTarget : NSObject <NSTextFieldDelegate, NSTextViewDelegate, NSMenuDelegate>
@property (nonatomic) QaplaMac::Widget* widget;
- (void)buttonClicked:(id)sender;
- (void)checkboxToggled:(id)sender;
- (void)textEntered:(id)sender;
- (void)stepperChanged:(id)sender;
- (void)sliderChanged:(id)sender;
- (void)popUpChosen:(id)sender;
@end

/** @brief A section header: a bar with a disclosure arrow that opens and closes its content. */
@interface QPSectionView : NSView
@property (nonatomic) QaplaMac::Widget* widget;
@property (nonatomic, copy) NSString* title;
@property (nonatomic) BOOL open;
@property (nonatomic) BOOL leaf;
@property (nonatomic) BOOL selected;
@property (nonatomic) BOOL enabled;
@end

/** @brief One entry of a list that can be chosen. */
@interface QPSelectableView : NSView
@property (nonatomic) QaplaMac::Widget* widget;
@property (nonatomic, copy) NSString* title;
@property (nonatomic) BOOL selected;
@property (nonatomic) BOOL enabled;
@end

/** @brief The strip of a tab bar: the tabs, their close buttons and the button that adds one. */
@interface QPTabStripView : NSView
@property (nonatomic) QaplaMac::Widget* widget;
- (void)setTabs:(const std::vector<QaplaMac::TabInfo>&)tabs
       selected:(std::uint64_t)selected
      addButton:(BOOL)addButton;
@end

/** @brief The handle between two panes. */
@interface QPSplitterView : NSView
@property (nonatomic) QaplaMac::Widget* widget;
@property (nonatomic) BOOL vertical;
@property (nonatomic) BOOL interactive;
@end

/** @brief A scroll view that may refuse the scroll wheel, as a pane that scrolls itself does. */
@interface QPScrollView : NSScrollView
@property (nonatomic) BOOL ignoresScrollWheel;
@end

/** @brief The window's content view: the root of the overlays, and a home for copy and paste. */
@interface QPRootView : NSView
@end

namespace QaplaMac {

    /** @brief A table widget: the scroll view to place, kept with its data source. */
    NSView* makeTableView(Widget& widget);

    /** @brief Brings the table view up to date with the table's rows and the frame it has. */
    void updateTableView(Widget& widget, QaplaWindows::ImGuiTable& table, NSRect frame);

    /** @brief Lets go of a table widget's data source. */
    void releaseTableView(Widget& widget);

} // namespace QaplaMac
