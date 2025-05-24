#pragma once
/*
For a 1kg (F.S.) load cell:
Non-linearity (linearity error): 0.05% F.S.
We might see °¿0.0005kg=0.5g difference from the °∞true°± weight.

Lag (hysteresis): 0.05% F.S.
At 0.6 kg applied, you might read 0.600 kg loading but 0.5995 kg unloading. (0.5g difference)

Repeatability: 0.05% F.S.
If we put and remove the same weight multiple times, readings will vary by no more than °¿0.0005kg=0.5g

Creep: 0.05% F.S./3 min
Hang 1 kg, hold it perfectly still; after 3 minutes the reading might slowly shift by up to °¿0.0005kg=0.5g

Temperature sensitivity drift: 0.05% F.S/10°„C
If room warms from 20 °„C °˙ 30 °„C, reading could jump by ~0.0005kg=0.5g
*/

template<int windowSize=20>
class LoadCell {
public:
  /**
   * @param changeThreshold  Minimum change in window-averaged weight to trigger detection
   * @param stabilityMargin  Maximum deviation in window to declare stabilization
   */
  LoadCell(float changeThreshold, float stabilityMargin)
    : _windowSize(windowSize),
      _changeThreshold(changeThreshold),
      _stabilityMargin(stabilityMargin),
      _idx(0), _count(0),
      _lastWindowAverage(0.0f),
      _lastStableWeight(0.0f),
      _stabilized(false),
      _changeDetected(false) {
  }

  /**
   * Add a new raw weight reading to the detector. Call at the sampling rate (e.g., 10?Hz).
   */
  void addMeasurement(float weight) {
    _buffer[_idx] = weight;
    _idx = (_idx + 1) % _windowSize;
    if (_count < _windowSize) {
      _count++;
      return;
    }

    // Compute mean over the full window
    float sum = 0;
    for (int i = 0; i < _windowSize; i++) {
      sum += _buffer[i];
    }
    float mean = sum / _windowSize;
    _lastWindowAverage = mean;

    // Detect change from last stable weight
    if (!_changeDetected) {
      if (fabs(mean - _lastStableWeight) > _changeThreshold) {
        _changeDetected = true;
        _stabilized = false;
      }
    }

    // If change detected, check for stabilization
    if (_changeDetected) {
      float maxDev = 0;
      for (int i = 0; i < _windowSize; i++) {
        float d = fabs(_buffer[i] - mean);
        if (d > maxDev) maxDev = d;
      }
      if (maxDev < _stabilityMargin) {
        // New stable weight found
        _lastStableWeight = mean;
        _stabilized = true;
        _changeDetected = false;
      } else {
        _stabilized = false;
      }
    }
  }

  bool isStabilized() const {
    return _stabilized;
  }

  /**
   * @return the most recent stabilized weight (valid after isStabilized() is true)
   */
  float getLastStabilizedWeight() const {
    return _lastStableWeight;
  }
  
  float getWindowAverage() const {
    return _lastWindowAverage;
  }

private:
  int _windowSize;
  float _changeThreshold;
  float _stabilityMargin;
  float _buffer[windowSize];
  int _idx;
  int _count;

  float _lastStableWeight;
  float _lastWindowAverage;
  bool _stabilized;
  bool _changeDetected;
};

template class LoadCell<10>;
template class LoadCell<20>;
template class LoadCell<30>;

