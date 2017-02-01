#define TRACKER_OPERATOR_ =
#define TRACKER_NOOP_ TRACKER_NOOP_RIGHTHAND_EQUAL_
#include "tracker_operator.hpp"
#undef TRACKER_OPERATOR_
#undef TRACKER_NOOP_

#define TRACKER_OPERATOR_ +=
#define TRACKER_NOOP_ TRACKER_NOOP_RIGHTHAND_ZERO_
#include "tracker_operator.hpp"
#undef TRACKER_OPERATOR_
#undef TRACKER_NOOP_

#define TRACKER_OPERATOR_ -=
#define TRACKER_NOOP_ TRACKER_NOOP_RIGHTHAND_ZERO_
#include "tracker_operator.hpp"
#undef TRACKER_OPERATOR_
#undef TRACKER_NOOP_

#define TRACKER_OPERATOR_ *=
#define TRACKER_NOOP_ TRACKER_NOOP_RIGHTHAND_ONE_
#include "tracker_operator.hpp"
#undef TRACKER_OPERATOR_
#undef TRACKER_NOOP_

#define TRACKER_OPERATOR_ /=
#define TRACKER_NOOP_ TRACKER_NOOP_RIGHTHAND_ONE_
#include "tracker_operator.hpp"
#undef TRACKER_OPERATOR_
#undef TRACKER_NOOP_

#define TRACKER_OPERATOR_ |=
#define TRACKER_NOOP_ TRACKER_NOOP_RIGHTHAND_ZERO_
#include "tracker_operator.hpp"
#undef TRACKER_OPERATOR_
#undef TRACKER_NOOP_

#define TRACKER_OPERATOR_ &=
#define TRACKER_NOOP_ TRACKER_NOOP_RIGHTHAND_EQUAL_
#include "tracker_operator.hpp"
#undef TRACKER_OPERATOR_
#undef TRACKER_NOOP_

#define TRACKER_OPERATOR_ ^=
#define TRACKER_NOOP_ TRACKER_NOOP_RIGHTHAND_ZERO_
#include "tracker_operator.hpp"
#undef TRACKER_OPERATOR_
#undef TRACKER_NOOP_

#define TRACKER_OPERATOR_ <<=
#define TRACKER_NOOP_ TRACKER_NOOP_RIGHTHAND_ZERO_
#include "tracker_operator.hpp"
#undef TRACKER_OPERATOR_
#undef TRACKER_NOOP_

#define TRACKER_OPERATOR_ >>=
#define TRACKER_NOOP_ TRACKER_NOOP_RIGHTHAND_ZERO_
#include "tracker_operator.hpp"
#undef TRACKER_OPERATOR_
#undef TRACKER_NOOP_
