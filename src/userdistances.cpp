/***********************************************************************

Vorosketch User Distances Demo by Herman Haverkort, 4 March 2023

************************************************************************

Copyright 2023 Herman Johannes Haverkort

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

This file is designed to be used with Vorosketch v0.31 BETA; it might
work with later versions too. New versions may be available from
http://herman.haverkort.net.


***********************************************************************/

// This file defines some distances as examples of what could be user-
// defined distances, including some distances that might be included
// into Vorosketch proper later but which are still subject to change.

// When Vorosketch is compiled with the -D INCLUDE_USER_DISTANCES option,
// the following macro is included to register distance definitions from
// this file with the parser for the -m option:

#include "vorosketch-037.h"

#define MAKEUSERDISTANCE                                                       \
  if (dname == "tspdistance")                                                  \
    return new TspDistance();                                                  \
  if (dname == "mydistance")                                                   \
    return new MyDistance();                                                   \
  if (dname == "@mydistance")                                                  \
    return new cMyDistance();                                                  \
  if (dname == "wave")                                                         \
    return new WaveDistance();                                                 \
  if (dname == "@wiggle")                                                      \
    return new cWiggleDistance();                                              \
  if (dname == "disk" || dname == "euclidean disk")                            \
    return new EuclideanDiskDistance();                                        \
  if (dname == "throw via cosite")                                             \
    return new ThrowViaCositeDistance();                                       \
  if (dname == "throw to cosite")                                              \
    return new ThrowToCositeDistance();

namespace vorosketch {

class MyDistance : public PointDistance {
public:
  virtual const std::string description() const { return "mydistance"; }

  virtual DOUBLE distanceToPointSite(const Point &p, const PointSite *s) const {
    return (p - s->location).length();
  }
};

class cMyDistance : public Distance {
public:
  virtual const std::string description() const { return "@mydistance"; }

  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const {
    return p.length();
  }
};

class WaveDistance : public SetDistance {
public:
  virtual const std::string description() const { return "wave"; }

  virtual DOUBLE distanceToPointSite(const Point &p, const PointSite *s) const {
    DOUBLE d = EuclideanDistance().distanceToPointSite(p, s);
    return (1.0 + (sin(d * 10 - PI * 0.5) - 1.0) * pow(0.1, d) * 0.5);
  }

  virtual DOUBLE distanceToSegmentSite(const Point &p,
                                       const SegmentSite *s) const {
    DOUBLE d = EuclideanDistance().distanceToSegmentSite(p, s);
    return (1.0 + (sin(d * 10 - PI * 0.5) - 1.0) * pow(0.1, d) * 0.5);
  }
};

class cWiggleDistance : public FieldDistance {
public:
  virtual const std::string description() const { return "@wiggle"; }

  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const {
    DOUBLE b = (p.inOrigin() ? 0 : atan2(p.y, p.x));
    DOUBLE wiggle = 0;
    for (int i = 4; i <= 32; i *= 4)
      wiggle += sin(b * i + HALFPI / i) / i;
    for (int i = 8; i <= 32; i *= 4)
      wiggle -= sin(b * i + HALFPI / i) / i;
    return wiggle;
  }

  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const {
    range.set(-0.5, 0.5);
  }
};

class EuclideanDiskDistance : public PointDistance {
public:
  virtual const std::string description() const { return "euclidean disk"; }

  virtual DOUBLE distanceToPointSite(const Point &p, const PointSite *s) const {
    if (p.length() >= 1 || s->distanceFromOrigin >= 1)
      return NAN;
    return (p / ((DOUBLE)1.0 - p.length()) -
            s->location / ((DOUBLE)1.0 - s->distanceFromOrigin))
        .length();
  }
};

class ThrowToCositeDistance : public RootedVectorDistance {
public:
  virtual const std::string description() const { return "throw to cosite"; }

  virtual DOUBLE distanceToRootedVectorSite(const Point &p,
                                            const RootedVectorSite *s) const {
    // squared speed that we should give a ball at s->location to reach p and
    // then the target point of the site vector, in that order. the measure
    // scales linearly with the gravity acceleration, so that it does not
    // matter; for ease of calculation we will take it to be 2;
    // if we scale all distances, the measure scales with the same factor

    // calculate position of p relative to s->location,
    // under scaling with factor 1/s->length
    Point relp = (p - s->location) * s->invLength;

    // handle the case of balls thrown (or dropped) vertically:
    if (s->direction.x == 0) {
      if (relp.x != 0)
        return INF;
      if (relp.y < std::min(s->direction.y, (DOUBLE)0))
        return INF;
      // throw it up high enough to reach the highest of p and the target point:
      return std::max(std::max(relp.y, s->direction.y), (DOUBLE)0) * 4 *
             s->length;
    }

    // check if p lies horizontally on the way from s to the target point:
    if (relp.x == 0 || relp.x == s->direction.x)
      return INF;
    if ((0 < relp.x) != (relp.x < s->direction.x))
      return INF;

    // so now either 0 < relp.x < s->direction.x or s->direction.x < relp.x < 0
    // calculate horizontal speed component, squared:
    DOUBLE sqvx =
        (s->direction.x - relp.x) / (relp.ySlope() - s->direction.ySlope());

    // if this number is negative, then it is not possible (p lies under the
    // line from site to target):
    if (sqvx < 0)
      return INF;

    // calculate vertical speed component, squared:
    DOUBLE sqvy = sqvx * sqr(relp.ySlope()) + sqr(relp.x) / sqvx + relp.y * 2;

    // add up and scale back up:
    return (sqvx + sqvy) * s->length;
  }
};

class ThrowViaCositeDistance : public RootedVectorDistance {
public:
  virtual const std::string description() const { return "throw via cosite"; }

  virtual DOUBLE distanceToRootedVectorSite(const Point &p,
                                            const RootedVectorSite *s) const {
    // squared speed that we should give a ball at s->location to reach the
    // target point of the site vector and then p, in that order. the measure
    // scales linearly with the gravity acceleration, so that it does not
    // matter; for ease of calculation we will take it to be 2;
    // if we scale all distances, the measure scales with the same factor

    // calculate position of p relative to s->location,
    // under scaling with factor 1/s->length
    Point relp = (p - s->location) * s->invLength;

    // handle the case of balls thrown (or dropped) vertically:
    if (s->direction.x == 0) {
      if (relp.x != 0)
        return INF;
      if (s->direction.y < 0 && relp.y > s->direction.y)
        // first down, then back up is not possible:
        return INF;
      // throw it up high enough to reach the highest of p and the target point:
      return std::max(std::max(relp.y, s->direction.y), (DOUBLE)0) * 4 *
             s->length;
    }

    // check if the target point lies horizontally on the way from s to p:
    if (relp.x == 0 || relp.x == s->direction.x)
      return INF;
    if ((0 < s->direction.x) != (s->direction.x < relp.x))
      return INF;

    // so now either 0 < s->direction.x < relp.x or relp.x < s->direction.x < 0
    // calculate horizontal speed component, squared:
    DOUBLE sqvx =
        (s->direction.x - relp.x) / (relp.ySlope() - s->direction.ySlope());

    // if this number is negative, then it is not possible (p lies above the
    // line from site to target):
    if (sqvx < 0)
      return INF;

    // calculate vertical speed component, squared:
    DOUBLE sqvy = sqvx * sqr(relp.ySlope()) + sqr(relp.x) / sqvx + relp.y * 2;

    // add up and scale back up:
    return (sqvx + sqvy) * s->length;
  }
};

class TspDistance : public SegmentDistance {
public:
  virtual const std::string description() const { return "TspDistance"; }
  virtual DOUBLE distanceToSegmentSite(const Point &p,
                                       const SegmentSite *s) const {
    DOUBLE l = s->getTspTourLength();
    // std::cerr << "\rtspTourlength: "<< l << std::endl;
    return (s->origin - p).length() + (p - s->destination).length() +
           s->getTspTourLength();
  }
  void boundDistanceToSegmentSite(const Point &centre, DOUBLE halfwidth,
                                  const SegmentSite *s, Range &range) const {
    DOUBLE distance = distanceToSegmentSite(centre, s);
    static const DOUBLE TWOROOTTWO = sqrt(2) * 2;
    DOUBLE maxChange = halfwidth * TWOROOTTWO;
    range.lowerBound = std::max(distance - maxChange, (DOUBLE)0);
    range.upperBound = distance + maxChange;
  }
};
} // namespace vorosketch
