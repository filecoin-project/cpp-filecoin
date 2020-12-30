/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/big_int.hpp"

namespace fc::common::math {
  using primitives::BigInt;

  /**
   * lambda = ln(2) / (6 * epochsInYear)
   * for Q.128: int(lambda * 2^128)
   * Calculation here:
   * https://www.wolframalpha.com/input/?i=IntegerPart%5BLog%5B2%5D+%2F+%286+*+%281+year+%2F+30+seconds%29%29+*+2%5E128%5D
   */
  static const BigInt kLambda{"37396271439864487274534522888786"};

  static constexpr uint kPrecision128 = 128;
  static constexpr uint kPrecision256 = 256;

  /**
   * Parameters are in integer format, coefficients are *2^-128 of that so we
   * can just load them if we treat them as Q.128
   */
  static const std::vector<BigInt> kExpNumCoef{
      BigInt{"-648770010757830093818553637600"},
      BigInt{"67469480939593786226847644286976"},
      BigInt{"-3197587544499098424029388939001856"},
      BigInt{"89244641121992890118377641805348864"},
      BigInt{"-1579656163641440567800982336819953664"},
      BigInt{"17685496037279256458459817590917169152"},
      BigInt{"-115682590513835356866803355398940131328"},
      BigInt{"340282366920938463463374607431768211456"},
  };

  static const std::vector<BigInt> kExpDenoCoef{
      BigInt{"1225524182432722209606361"},
      BigInt{"114095592300906098243859450"},
      BigInt{"5665570424063336070530214243"},
      BigInt{"194450132448609991765137938448"},
      BigInt{"5068267641632683791026134915072"},
      BigInt{"104716890604972796896895427629056"},
      BigInt{"1748338658439454459487681798864896"},
      BigInt{"23704654329841312470660182937960448"},
      BigInt{"259380097567996910282699886670381056"},
      BigInt{"2250336698853390384720606936038375424"},
      BigInt{"14978272436876548034486263159246028800"},
      BigInt{"72144088983913131323343765784380833792"},
      BigInt{"224599776407103106596571252037123047424"},
      BigInt{"340282366920938463463374607431768211456"},
  };

  /**
   * Evaluates a polynomial
   * Coefficients should be ordered from the highest order coefficient to the
   * lowest.
   * @param p in format set in precision param
   * @param x in format set in precision param
   * @param precision - precision for format, defauld Q.128
   * @return output is in Q.128
   */
  inline BigInt polyval(const std::vector<BigInt> &p,
                        const BigInt &x,
                        uint precision = kPrecision128) {
    // evaluation using Horner's method
    BigInt res{0};
    for (const auto &c : p) {
      res = ((res * x) >> precision) + (c << (precision - kPrecision128));
    }
    return res;
  }

  /**
   * Computes e^-x
   * It is most precise within [0, 1.725) range, where error is less
   * than 3.4e-30. Over the [0, 5) range its error is less than 4.6e-15
   * @param x Q.128 format
   * @return Q.128 format
   */
  inline BigInt expneg(const BigInt &x, uint precision = kPrecision128) {
    // exp is approximated by rational function
    // polynomials of the rational function are evaluated using Horner's method
    return bigdiv(polyval(kExpNumCoef, x, precision) << precision,
                  polyval(kExpDenoCoef, x, precision));
  }
}  // namespace fc::common::math
