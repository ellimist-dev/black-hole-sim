#include "theme.hh"

#include <raydom/extras/theme.h>

using namespace rd::literals;

namespace theme {

void apply(const rd::Context& ctx) {
  const rdx_classes* t = rdx_theme(ctx.get());
  if (!t) return;
  auto     cls  = [&ctx](rd_class c) { return rd::Class(ctx.get(), c); };
  rd_color ring = rd::rgba(249, 38, 114, 200), glow = rd::rgba(249, 38, 114, 90);

  cls(t->label).set(rd::Style().color(rd::hex(kText)));

  cls(t->check_box).on(RD_STATE_HOVER, rd::Style().border_color(rd::hex(kSoft)));
  cls(t->check_box).on(RD_STATE_CHECKED, rd::Style().background(rd::hex(kPink)).border_color(rd::hex(kPink)));

  cls(t->switch_track)
      .set(rd::Style()
               .width(36_px)
               .height(20_px)
               .padding(2_px)
               .flex_direction(RD_FLEX_ROW)
               .align_items(RD_ALIGN_CENTER)
               .justify_content(RD_ALIGN_START)
               .border_radius(10_px)
               .background(rd::hex(kRaised))
               .transition_duration(120_ms)
               .transition_easing(RD_EASING_EASE_OUT))
      .on(RD_STATE_HOVER, rd::Style().background(rd::hex(kHover)))
      .on(RD_STATE_CHECKED, rd::Style().background(rd::hex(kPink)));

  cls(t->slider).on(RD_STATE_FOCUS_VISIBLE, rd::Style().box_shadow(0, 0, 0, ring, 2));
  cls(t->slider_track).set(rd::Style().height(4_px).border_radius(2_px).background(rd::hex(kRaised)));
  cls(t->slider_fill).set(rd::Style()
                              .position(RD_POSITION_ABSOLUTE)
                              .inset(0_px, rd::auto_, rd::auto_, 0_px)
                              .height(100_pct)
                              .border_radius(2_px)
                              .background(rd::hex(kPink)));
  cls(t->slider_thumb)
      .on(RD_STATE_HOVER, rd::Style().box_shadow(0, 0, 0, glow, 5))
      .on(RD_STATE_ACTIVE, rd::Style().background(rd::hex(kPinkHi)).box_shadow(0, 0, 0, glow, 5));

  cls(t->split_handle)
      .set(rd::Style()
               .width(6_px)
               .background(rd::hex(kLine))
               .cursor(RD_CURSOR_EW_RESIZE)
               .focusable(RD_FOCUSABLE_TAB)
               .transition_duration(120_ms)
               .transition_easing(RD_EASING_EASE_OUT)
               .transition_properties(RD_TRANSITION_COLOR))
      .on(RD_STATE_HOVER, rd::Style().background(rd::hex(kHover)))
      .on(RD_STATE_ACTIVE, rd::Style().background(rd::hex(kPink)))
      .on(RD_STATE_FOCUS_VISIBLE, rd::Style().background(rd::hex(kPink)));

  cls(t->number)
      .set(rd::Style()
               .flex_direction(RD_FLEX_ROW)
               .width(96_px)
               .border_width(1_px)
               .border_color(rd::hex(kHover))
               .border_radius(6_px)
               .background(rd::hex(kBg))
               .overflow_x(RD_OVERFLOW_HIDDEN)
               .overflow_y(RD_OVERFLOW_HIDDEN)
               .transition_duration(120_ms)
               .transition_properties(RD_TRANSITION_COLOR))
      .on(RD_STATE_HOVER, rd::Style().border_color(rd::hex(kSoft)))
      .on(RD_STATE_FOCUS_WITHIN, rd::Style().border_color(rd::hex(kPink)));
  cls(t->number_input)
      .set(rd::Style().flex(1).min_width(0_px).padding(4_px, 8_px).color(rd::hex(kText)).white_space(RD_WHITE_SPACE_NOWRAP));
  cls(t->number_steppers)
      .set(rd::Style()
               .width(18_px)
               .flex_shrink(rd::num(0))
               .border_width(0_px, 0_px, 0_px, 1_px)
               .border_color(rd::hex(kHover))
               .user_select(RD_USER_SELECT_NONE));
  cls(t->number_step)
      .on(RD_STATE_HOVER, rd::Style().background(rd::hex(kRaised)).color(rd::hex(kPink)))
      .on(RD_STATE_ACTIVE, rd::Style().background(rd::hex(kHover)).color(rd::hex(kPinkHi)));

  cls(t->select)
      .set(rd::Style()
               .flex_direction(RD_FLEX_ROW)
               .align_items(RD_ALIGN_CENTER)
               .column_gap(8_px)
               .min_width(160_px)
               .padding(7_px, 10_px)
               .border_width(1_px)
               .border_color(rd::hex(kHover))
               .border_radius(8_px)
               .background(rd::hex(kRaised))
               .cursor(RD_CURSOR_POINTER)
               .user_select(RD_USER_SELECT_NONE)
               .transition_duration(120_ms)
               .transition_easing(RD_EASING_EASE_OUT))
      .on(RD_STATE_HOVER, rd::Style().border_color(rd::hex(kSoft)))
      .on(RD_STATE_FOCUS_VISIBLE, rd::Style().box_shadow(0, 0, 0, ring, 2));
  cls(t->select_chevron).set(rd::Style().height(14_px).color(rd::hex(kPink)));

  cls(t->menu).set(rd::Style()
                       .position(RD_POSITION_ABSOLUTE)
                       .min_width(180_px)
                       .max_height(320_px)
                       .overflow_y(RD_OVERFLOW_SCROLL)
                       .padding(4_px)
                       .row_gap(1_px)
                       .border_width(1_px)
                       .border_color(rd::hex(kHover))
                       .border_radius(8_px)
                       .background(rd::hex(kPanel))
                       .box_shadow(0, 10, 28, rd::rgba(0, 0, 0, 160))
                       .color(rd::hex(kText))
                       .focusable(RD_FOCUSABLE_POINTER)
                       .user_select(RD_USER_SELECT_NONE));
  cls(t->menu_item)
      .on(RD_STATE_HOVER, rd::Style().background(rd::hex(kRaised)))
      .on(RD_STATE_FOCUS, rd::Style().background(rd::hex(kRaised)));
  cls(t->menu_check).set(rd::Style().width(14_px).height(14_px).color(rd::hex(kPink)).opacity(rd::num(0)));

  cls(t->tooltip).set(rd::Style()
                          .position(RD_POSITION_ABSOLUTE)
                          .max_width(280_px)
                          .padding(5_px, 9_px)
                          .border_radius(6_px)
                          .border_width(1_px)
                          .border_color(rd::hex(kHover))
                          .background(rd::hex(kRaised))
                          .box_shadow(0, 4, 12, rd::rgba(0, 0, 0, 140))
                          .color(rd::hex(kText))
                          .font_size(12_px)
                          .pointer_events(RD_POINTER_EVENTS_NONE));
}

}  // namespace theme
