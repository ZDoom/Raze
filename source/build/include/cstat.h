#pragma once
// nobody uses these. What's so cool about naked numbers? :(
#if 0
// system defines for status bits
#define CEILING_STAT_PLAX           BIT(0)
#define CEILING_STAT_SLOPE          BIT(1)
#define CEILING_STAT_SWAPXY         BIT(2)
#define CEILING_STAT_SMOOSH         BIT(3)
#define CEILING_STAT_XFLIP          BIT(4)
#define CEILING_STAT_YFLIP          BIT(5)
#define CEILING_STAT_RELATIVE       BIT(6)
#define CEILING_STAT_TYPE_MASK     (BIT(7)|BIT(8))
#define CEILING_STAT_MASKED         BIT(7)
#define CEILING_STAT_TRANS          BIT(8)
#define CEILING_STAT_TRANS_FLIP     (BIT(7)|BIT(8))
#define CEILING_STAT_FAF_BLOCK_HITSCAN      BIT(15)

#define FLOOR_STAT_PLAX           BIT(0)
#define FLOOR_STAT_SLOPE          BIT(1)
#define FLOOR_STAT_SWAPXY         BIT(2)
#define FLOOR_STAT_SMOOSH         BIT(3)
#define FLOOR_STAT_XFLIP          BIT(4)
#define FLOOR_STAT_YFLIP          BIT(5)
#define FLOOR_STAT_RELATIVE       BIT(6)
#define FLOOR_STAT_TYPE_MASK     (BIT(7)|BIT(8))
#define FLOOR_STAT_MASKED         BIT(7)
#define FLOOR_STAT_TRANS          BIT(8)
#define FLOOR_STAT_TRANS_FLIP     (BIT(7)|BIT(8))
#define FLOOR_STAT_FAF_BLOCK_HITSCAN      BIT(15)

#define CSTAT_WALL_BLOCK            BIT(0)
#define CSTAT_WALL_BOTTOM_SWAP      BIT(1)
#define CSTAT_WALL_ALIGN_BOTTOM     BIT(2)
#define CSTAT_WALL_XFLIP            BIT(3)
#define CSTAT_WALL_MASKED           BIT(4)
#define CSTAT_WALL_1WAY             BIT(5)
#define CSTAT_WALL_BLOCK_HITSCAN    BIT(6)
#define CSTAT_WALL_TRANSLUCENT      BIT(7)
#define CSTAT_WALL_YFLIP            BIT(8)
#define CSTAT_WALL_TRANS_FLIP       BIT(9)
#define CSTAT_WALL_BLOCK_ACTOR (BIT(14)) // my def
#define CSTAT_WALL_WARP_HITSCAN (BIT(15)) // my def
#endif
