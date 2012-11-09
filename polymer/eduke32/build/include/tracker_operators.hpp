#define __TRACKER_OPERATOR =
#define __TRACKER_NOOP __TRACKER_NOOP_RIGHTHAND_EQUAL
#include "tracker_operator.hpp"
#undef __TRACKER_OPERATOR
#undef __TRACKER_NOOP

#define __TRACKER_OPERATOR +=
#define __TRACKER_NOOP __TRACKER_NOOP_RIGHTHAND_ZERO
#include "tracker_operator.hpp"
#undef __TRACKER_OPERATOR
#undef __TRACKER_NOOP

#define __TRACKER_OPERATOR -=
#define __TRACKER_NOOP __TRACKER_NOOP_RIGHTHAND_ZERO
#include "tracker_operator.hpp"
#undef __TRACKER_OPERATOR
#undef __TRACKER_NOOP

#define __TRACKER_OPERATOR *=
#define __TRACKER_NOOP __TRACKER_NOOP_RIGHTHAND_ONE
#include "tracker_operator.hpp"
#undef __TRACKER_OPERATOR
#undef __TRACKER_NOOP

#define __TRACKER_OPERATOR /=
#define __TRACKER_NOOP __TRACKER_NOOP_RIGHTHAND_ONE
#include "tracker_operator.hpp"
#undef __TRACKER_OPERATOR
#undef __TRACKER_NOOP

#define __TRACKER_OPERATOR |=
#define __TRACKER_NOOP __TRACKER_NOOP_RIGHTHAND_ZERO
#include "tracker_operator.hpp"
#undef __TRACKER_OPERATOR
#undef __TRACKER_NOOP

#define __TRACKER_OPERATOR &=
#define __TRACKER_NOOP __TRACKER_NOOP_RIGHTHAND_EQUAL
#include "tracker_operator.hpp"
#undef __TRACKER_OPERATOR
#undef __TRACKER_NOOP

#define __TRACKER_OPERATOR ^=
#define __TRACKER_NOOP __TRACKER_NOOP_RIGHTHAND_ZERO
#include "tracker_operator.hpp"
#undef __TRACKER_OPERATOR
#undef __TRACKER_NOOP

#define __TRACKER_OPERATOR <<=
#define __TRACKER_NOOP __TRACKER_NOOP_RIGHTHAND_ZERO
#include "tracker_operator.hpp"
#undef __TRACKER_OPERATOR
#undef __TRACKER_NOOP

#define __TRACKER_OPERATOR >>=
#define __TRACKER_NOOP __TRACKER_NOOP_RIGHTHAND_ZERO
#include "tracker_operator.hpp"
#undef __TRACKER_OPERATOR
#undef __TRACKER_NOOP
