#pragma once
/*  
 *  This header generates hexBoard
 *  layouts based on menu selections.
 *  Lots of dependencies but they're
 *  all common to the main .ino file
 *  so rather than list them all, just
 *  make sure this header occurs at the
 *  end after other declarations.
 */

template <typename T1, typename T2>
void update_if_closer_to_zero(T1& LHS, const T2& RHS) {
  if (std::abs(RHS) < std::abs(LHS)) {LHS = RHS;}
}

// cache value of log(3/2) / log(2)
const double perfectFifth = 0.5849625007211562;

void generate_and_apply_EDO_layout(double octave, int EDO, int A_span, int B_span) {
  int spanFifth = round(EDO * perfectFifth);
  // for 13EDO and 18EDO it is preferable to round down
  // to the "flatter" fifth and not up to the "sharper" fifth
  if ((EDO == 13) || (EDO == 18)) { --spanFifth; }
  int spanFourth = EDO - spanFifth;
  int spanSharp  = 7 * spanFifth - 4 * EDO;
  int spanMajor2 = 2 * spanFifth - EDO;
  int spanMinor2 = spanMajor2 - spanSharp;
  int spanMinor3 = spanFourth - spanMajor2;
  for (auto& n : hexBoard.btn) {
    if (!n.isBtn) continue;
    if (!n.isNote) continue; // for now assuming cmd btns are unchanged
    n.scaleDegree = A_span * n.A_steps + B_span * n.B_steps; 
    n.midiPitch += n.scaleDegree * (octave / 100.0 / EDO);
    // scale by octave
    n.scaleEquave = 0;
    while (n.scaleDegree < 0) {
      --(n.scaleEquave);
      n.scaleDegree += EDO;
    }
    while (n.scaleDegree >= EDO) {
      ++(n.scaleEquave);
      n.scaleDegree -= EDO;
    }
    if (EDO < 10) { // algorithm doesn't really work below 10 EDO
      n.paletteNum = n.scaleDegree;
    } else {
      // the white keys are tier 0 (C D E F G A B)
      // the black keys are tier 1 (C#, Db, etc.)
      // in larger EDOs you get other tiers like:
      // tier -1: E# B# Fb Cb if they're separate
      // tier 2/-2 or larger for other microtonal steps
      // the algorithm works by knowing how to spell
      // D and E, then treating all C F G & A's like D
      // and treating B like E. we are assuming the
      // "key" is "C", so that C = zero steps.
      
      // G, A, and B are spelled the same as C, D, and E.
      // so take the scale degree modulo the 5th.
      int stepsForPalette = n.scaleDegree % spanFifth;
            // C is spelled like D, but D is now going to be zero.
            if (stepsForPalette >= spanMajor2) { stepsForPalette -= spanMajor2; }
            // G is spelled like F.
            if (stepsForPalette >= spanFourth ) { stepsForPalette -= spanMajor2; }
            // F is spelled like D.
            if (stepsForPalette >= spanMinor3 ) { stepsForPalette -= spanMinor3; }
            // C, D, F, G, and A are white keys.
            if (stepsForPalette == 0)                       { n.paletteNum =  0; }
            // E and B are white keys.
      else  if (stepsForPalette == spanMajor2)              { n.paletteNum =  0; }
            // C#, D#, F#, G#, and A# are black keys.
      else  if (stepsForPalette == spanSharp)               { n.paletteNum =  1; }
            // Db, Eb, Gb, Ab, and Bb are black keys.
      else  if (stepsForPalette == spanMinor2)              { n.paletteNum =  1; }
            // E# and B# get a different color if needed.
      else  if (stepsForPalette == spanMajor2 + spanSharp)  { n.paletteNum = -1; }
            // Fb and Cb get a different color if needed.
      else  if (stepsForPalette == spanMinor3 - spanSharp)  { n.paletteNum = -1; }
      else {
        // if the note isn't one of those, then find how many microtonal steps
        // away it is from a white or black key (not from E#/Fb).
        n.paletteNum = stepsForPalette;
        update_if_closer_to_zero(n.paletteNum, stepsForPalette - spanMinor3);
        update_if_closer_to_zero(n.paletteNum, stepsForPalette - spanMajor2);
        update_if_closer_to_zero(n.paletteNum, stepsForPalette - spanMinor2);
        update_if_closer_to_zero(n.paletteNum, stepsForPalette - spanSharp);
        // then increase by one so that you start counting 
        // at tier 2, 3, ... if sharp, or -2, -3, ... if flat.
        // e.g. D^ is one microtonal step sharp of D, tier +2.
        //      Dv is one microtonal step flat of D, tier -2.
        //      Bb^^ is two microtonal steps sharp of Bb, tier 3.
        n.paletteNum += (n.paletteNum > 0 ? 1 : -1);
      }
    }
  }
}


// recursive formula to generate MOS scale from large/small steps
// outputs a vector of booleans
// large step = TRUE/1   small step = FALSE/0
#include <vector>
#include <map>

uint gcd(const uint& _a, const uint& _b) {
  for (uint _f = _a - 1; _f > 1; --_f) {
    if (!((_a % _f) || (_b % _f))) {
      return _f;
    }
  }
  return 1;
}

struct MOS_Scale {
  std::vector<bool> steps;
  // inMode[Mode #][Lg steps][Sm steps]
  std::vector<std::vector<std::vector<bool>>> inMode;
  std::vector<std::vector<bool>> inMode_EDO_equiv;
  uint Lg;
  uint Sm;
  uint K; // GCD
  uint G; // bright generator
  uint modeCt;
  bool rational;
  uint underlyingEDO;

  float ratio_f;
  uint ratio_num_s;
  uint ratio_den_L;

  void determine_G_and_K() {
    K = gcd(Lg,Sm);
    G = 0;
    for (uint8_t m = 1; m < (Lg + Sm); ++m) {
      if ((Sm * m) % (Lg + Sm) == 1) {
        G = m;
        break;
      }
    }
    modeCt = (Lg + Sm) / K;
  }

  std::vector<bool> recursively_generate_MOS_scale(uint8_t L, uint8_t S) {
    std::vector<bool> result;
    if ((L == 1) || (S == 1)) {
      for (uint8_t i = 0; i < L; ++i) {
        result.emplace_back(true);
      }
      for (uint8_t i = 0; i < S; ++i) {
        result.emplace_back(false);
      }
      return result;
    } else {
      int K = gcd(L,S);
      if (K > 1) {
        result = recursively_generate_MOS_scale(L/K, S/K);
        std::vector<bool> repeatKtimes;
        for (uint8_t i = 0; i < K; ++i) {
          for (uint8_t j = 0; j < result.size(); ++j) {
            repeatKtimes.emplace_back(result[j]);
          }
        }
        return repeatKtimes;
      } else {
        uint8_t Mn = (L < S ? L : S);
        uint8_t Mx = (L > S ? L : S);
        uint8_t z = Mx % Mn;
        uint8_t w = Mn - z;
        uint8_t v = Mx / Mn; // floor, integer division
        std::vector<bool> preScale = recursively_generate_MOS_scale(z, w);
        if (L < S) {
          std::reverse(preScale.begin(), preScale.end());
          for (uint8_t i = 0; i < preScale.size(); ++i) {
            result.emplace_back(true);
            for (uint8_t j = 0; j < v + preScale[i]; ++j) {
              result.emplace_back(false);
            }
          }
        } else {
          for (uint8_t i = 0; i < preScale.size(); ++i) {
            for (uint8_t j = 0; j < v + preScale[i]; ++j) {
              result.emplace_back(true);
            }
            result.emplace_back(false);
          }
        }
      }
    }
    return result;
  }

  MOS_Scale(int argL, int argS) : Lg(argL), Sm(argS) {
    determine_G_and_K();
    steps = recursively_generate_MOS_scale(Lg,Sm);
  }

  void set_L_S_ratio(float r) {
    rational = false;
    ratio_f = r;
    underlyingEDO = 0;
  }

  void set_L_S_ratio(uint n_s, uint d_l) {
    rational = true;
    ratio_num_s = n_s;
    ratio_den_L = d_l;
    ratio_f = (float)n_s / (float)d_l;
    underlyingEDO = Sm * d_l + Lg * n_s;
    inMode_EDO_equiv.resize(modeCt, 
      std::vector<bool>(underlyingEDO, false));
  }

  std::vector<bool> get_mode(int mode) {
    std::vector<bool> result;
    for (uint8_t i = 0; i < (Lg + Sm); ++i) {
      result.emplace_back(steps[(i + (G * mode)) % (Lg + Sm)]);
    }
    return result;
  }  

  void determine_key_colors() {
    // initialize vector
    inMode.resize(modeCt,
      std::vector<std::vector<bool>>(Lg + 1,
        std::vector<bool>(Sm + 1, false)
    ));
    int iL; // iterate # lg steps
    int iS; // iterate # sm steps
    for (uint8_t m = 0; m < modeCt; ++m) {
      // traverse each mode of this scale
      std::vector<bool> thisMode = get_mode(m);
      iL = 0;
      iS = 0;
      for (uint8_t i = 0; i < Lg + Sm; ++i) {
        // traverse each step in this mode
        // and mark it TRUE
        inMode[m][iL][iS] = true;
        if (rational) {
          inMode_EDO_equiv[m][iL * ratio_num_s + iS * ratio_den_L] = true;
        }
        iL +=  thisMode[i];
        iS += !thisMode[i];
      }
    }
  }

};



void apply_MOS_layout ( Hex    A_span, // X = lg, Y = sm
                        Hex    B_span, // X = lg, Y = sm
                        const  MOS_Scale& _mos,
                        double period, int _mode) {
  float smCents = period / _mos.Lg * _mos.ratio_f + _mos.Sm;
  float lgCents = smCents * _mos.ratio_f;
  for (auto& n : hexBoard.btn) {
    if (!n.isBtn) continue;
    if (!n.isNote) continue; // for now assuming cmd btns are unchanged
    n.smallDegree = A_span.y * n.A_steps + B_span.y * n.B_steps;
    n.largeDegree = A_span.x * n.A_steps + B_span.x * n.B_steps;
    n.scaleEquave = 0;
    if (_mos.rational) {
      n.scaleDegree = n.largeDegree * _mos.ratio_num_s
                    + n.smallDegree * _mos.ratio_den_L;
      while (n.scaleDegree < 0) {
        --n.scaleEquave;
        n.smallDegree += _mos.Sm;
        n.largeDegree += _mos.Lg;
        n.scaleDegree += _mos.underlyingEDO;
      }
      while (n.scaleDegree >= _mos.underlyingEDO) {
        ++n.scaleEquave;
        n.smallDegree -= _mos.Sm;
        n.largeDegree -= _mos.Lg;
        n.scaleDegree -= _mos.underlyingEDO;
      }
      n.paletteNum = -1;
      if (_mos.inMode_EDO_equiv[_mode][n.scaleDegree]) {
        n.paletteNum = 0; // white key
      } else {
        for (uint i = 0; i < _mos.modeCt; ++i) {
          if (_mos.inMode_EDO_equiv[(_mode + i) % _mos.modeCt][n.scaleDegree]) {
            n.paletteNum = 1;
          }
        }
      }
    } else {
      while ((n.smallDegree < 0) || (n.largeDegree < 0)) {
        --n.scaleEquave;
        n.smallDegree += _mos.Sm;
        n.largeDegree += _mos.Lg;
      }
      while ((n.smallDegree >= _mos.Sm) && (n.largeDegree >= _mos.Lg)) {
        ++n.scaleEquave;
        n.smallDegree -= _mos.Sm;
        n.largeDegree -= _mos.Lg;
      }
      n.paletteNum = -1;
      if ((n.largeDegree <= _mos.Lg) && (n.smallDegree <= _mos.Sm)) {
        if (_mos.inMode[_mode][n.largeDegree][n.smallDegree]) {
          n.paletteNum = 0;
        } else {
          for (uint i = 0; i < _mos.modeCt; ++i) {
            if (_mos.inMode[(_mode + i) % _mos.modeCt][n.largeDegree][n.smallDegree]) {
              n.paletteNum = 1;
            }
          }
        }
      }
    }
    n.midiPitch += period * n.scaleEquave / 100.0;
    n.midiPitch += (smCents * n.smallDegree + lgCents * n.largeDegree) / 100.0;
  }
}

#include "color.h"

bool generate_layout(hexBoard_Setting_Array& refS) {
  Hex anchorHex(refS[_anchorX].i, refS[_anchorY].i);
  if (!hexBoard.in_bounds(anchorHex)) {
    return false; // 1) anchor hex must be valid -- in range and not a Cmd
  }
  Hex hexA = unitHex[refS[_axisA].i];
  Hex hexB = unitHex[refS[_axisB].i];
  if ((hexA == hexB) || (hexA == hexB * -1)) {
    return false; // 2) axes cannot be parallel
  }
  // express the root pitch as MIDI (note + cents/100)
  double anchorPitch = (double)refS[_anchorN].i + refS[_anchorC].d / 100.0;  
  if (refS[_equaveC].d <= 0.0) {
    return false; // 3) equave must be valid
  }
  for (auto& n : hexBoard.btn) {
    // cache the number of A and B steps for each button
    // relative to anchor hex
    Hex h = n.coord - anchorHex;
    if ((hexA == unitHex[dir_e]) || (hexA == unitHex[dir_w])) {
      n.B_steps  =  h.y                       / hexB.y;
      n.A_steps  = (h.x - n.B_steps * hexB.x) / hexA.x;
    } else if ((hexB == unitHex[dir_e]) || (hexB == unitHex[dir_w])) {
      n.A_steps  =  h.y                       / hexA.y;
      n.B_steps  = (h.x - n.A_steps * hexA.x) / hexB.x;
    } else {
      n.A_steps  =  h.x                           / hexA.x;
      n.B_steps  = (h.y - n.A_steps * hexA.y) / 2 / hexB.y;
      n.A_steps += (h.y - n.A_steps * hexA.y) / 2 / hexA.y;
    }
    // preload anchor pitch
    n.midiPitch = anchorPitch;
  }    
  // run the next steps based on what tuning system we're in
  switch (refS[_tuneSys].i) {
    case _tuneSys_normal: {
      generate_and_apply_EDO_layout(1200.0, 12, 
        refS[_eqStepA].i, refS[_eqStepB].i);
      break;
    }
    case _tuneSys_equal: {
      generate_and_apply_EDO_layout(refS[_equaveC].d, refS[_eqDivs].i, 
        refS[_eqStepA].i, refS[_eqStepB].i);
      break;
    }
    case _tuneSys_lg_sm: {
      Hex A_axis(refS[_lgStepA].i, refS[_smStepA].i);
      Hex B_axis(refS[_lgStepB].i, refS[_smStepB].i);
      MOS_Scale MOS(refS[_lgSteps].i, refS[_smSteps].i);
      if (refS[_lgToSmND].b) {
        MOS.set_L_S_ratio(refS[_lgToSmN].i, refS[_lgToSmD].i);
      } else {
        MOS.set_L_S_ratio(refS[_lgToSmR].d);
      }
      MOS.determine_key_colors();
      apply_MOS_layout(A_axis, B_axis, MOS, refS[_equaveC].d, refS[_modeLgSm].i);
      break;
    }
    case _tuneSys_just: {
      float JIcentsA = intervalToCents((float)refS[_JInumA].i / (float)refS[_JIdenA].i);
      float JIcentsB = intervalToCents((float)refS[_JInumB].i / (float)refS[_JIdenB].i);
      for (auto& n : hexBoard.btn) {
        if (!n.isBtn) continue;
        if (!n.isNote) continue; // for now assuming cmd btns are unchanged
        n.midiPitch += JIcentsA * n.A_steps / 100.0;
        n.midiPitch += JIcentsB * n.B_steps / 100.0;
        n.paletteNum = 0;
      }
      break;
    }
  }
  for (auto& n : hexBoard.btn) {
    n.frequency = MIDItoFreq(n.midiPitch);
    HSV paletteColor;
    switch (n.paletteNum) {
      case 0: {
        paletteColor.h = 0.0;
        paletteColor.s = 0.0;
        paletteColor.v = 1.0;
        break; 
      } // white key
      case 1: {
        paletteColor.h = 270.0;
        paletteColor.s = 1.0;
        paletteColor.v = 0.5;
        break;
      } // black key
      case -1: {
        paletteColor.h = 45.0;
        paletteColor.s = 1.0;
        paletteColor.v = 0.5; 
        break; 
      } // E#/Fb
      default: {
        paletteColor.h = 144.0 + 36.0 * n.paletteNum;
        paletteColor.s = 0.5;
        paletteColor.v = 0.5;
        break;
      }
    }
    paletteColor.v *= refS[_globlBrt].i / 255.0;
    n.LEDcodeBase = okhsv_to_neopixel_code(paletteColor);
  }
  // reflect transposition in frequencies

  // if MTS mode
  // enumerate all the calculated pitches
  // sort ascending, create MTS tuning table
  // assign MIDI note based on this

  // if not MTS mode
  // MIDI note is round midiPitch
  // calculate MPE pitch bend and store it

  // note coloring, this might take a while

  return true;
}
