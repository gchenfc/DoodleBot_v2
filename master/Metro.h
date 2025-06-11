#pragma once

class Metro {
 public:
  Metro(size_t interval) : last_ms_(millis()), interval_(interval) {}

  bool check() {
    bool ret = false;
    size_t cur_ms = millis();
    while (cur_ms - last_ms_ >= interval_) {
      ret = true;
      last_ms_ += interval_;
    }
    return ret;
  }

 private:
  size_t last_ms_;
  size_t interval_;
};
