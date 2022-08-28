/*
** interphelpers.h
**
** Interpolation helpers for use throughout Build games. 
**
**---------------------------------------------------------------------------
** Copyright 2022 Christoph Oelckers, Mitchell Richters
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#pragma once

#include "fixedhorizon.h"

inline constexpr int32_t interpolatedvalue(int32_t oval, int32_t val, double const smoothratio, int const scale = 16)
{
	return oval + MulScale(val - oval, int(smoothratio), scale);
}

inline constexpr int32_t interpolatedvalue(int32_t oval, int32_t val, int const smoothratio, int const scale = 16)
{
	return oval + MulScale(val - oval, smoothratio, scale);
}

inline constexpr double interpolatedvaluef(double oval, double val, double const smoothratio, int const scale = 16)
{
	return oval + MulScaleF(val - oval, smoothratio, scale);
}

inline constexpr int32_t interpolatedangle(int32_t oang, int32_t ang, double const smoothratio, int const scale = 16)
{
	return oang + MulScale(((ang + 1024 - oang) & 2047) - 1024, int(smoothratio), scale);
}

inline constexpr int32_t interpolatedangle(int32_t oang, int32_t ang, int const smoothratio, int const scale = 16)
{
	return oang + MulScale(((ang + 1024 - oang) & 2047) - 1024, smoothratio, scale);
}

inline DAngle interpolatedangle(DAngle oang, DAngle ang, double const smoothratio, int const scale = 16)
{
	return oang + (deltaangle(oang, ang) * smoothratio * (1. / (1 << scale)));
}

inline DAngle interpolatedangle(DAngle oang, DAngle ang, int const smoothratio, int const scale = 16)
{
	return oang + (deltaangle(oang, ang) * smoothratio * (1. / (1 << scale)));
}

inline constexpr fixedhoriz interpolatedhorizon(fixedhoriz oval, fixedhoriz val, double const smoothratio, int const scale = 16)
{
	return q16horiz(oval.asq16() + MulScale((val - oval).asq16(), int(smoothratio), scale));
}

inline constexpr fixedhoriz interpolatedhorizon(fixedhoriz oval, fixedhoriz val, int const smoothratio, int const scale = 16)
{
	return q16horiz(oval.asq16() + MulScale((val - oval).asq16(), smoothratio, scale));
}
