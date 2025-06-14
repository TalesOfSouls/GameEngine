/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_MOB_ACTION_H
#define COMS_MODELS_MOB_ACTION_H

// @todo Consider to create better ranges
//      This would allow easier checks during the game (e.g. can be healed if > X and < Y)
//      Checks to consider are:
//          - Can be healed
//          - Can be attacked
//          - Can attack
//          - Can escape/teleport/... (-> out of fight)
//          - Can cast
//          - Can call mount (e.g. not during falling or during fight)
//          - ...

// Don't render, ready for garbage collection and re-use
#define MOB_ACTION_INACTIVE 0x00

#define MOB_ACTION_LAG 0x01

#define MOB_ACTION_AAAA 0x02

#define MOB_ACTION_WALK 0x03
#define MOB_ACTION_RUN 0x04
#define MOB_ACTION_SNEAK 0x05
#define MOB_ACTION_CRAWL 0x06
#define MOB_ACTION_CLIMB 0x07
#define MOB_ACTION_SLIDE 0x08
#define MOB_ACTION_JUMP 0x09
#define MOB_ACTION_DUCK 0x0A
#define MOB_ACTION_FALL 0x0B

#define MOB_ACTION_TALK 0x0C
#define MOB_ACTION_ATTACK 0x0D
#define MOB_ACTION_DEFEND 0x0E
#define MOB_ACTION_BLOCK 0x0F
#define MOB_ACTION_DODGE 0x10
#define MOB_ACTION_DIE 0x11
#define MOB_ACTION_DEAD 0x12

#define MOB_ACTION_SWIM 0x13
#define MOB_ACTION_SWIM_HOVER 0x14

#define MOB_ACTION_DIVE 0x15
#define MOB_ACTION_DIVE_HOVER 0x16

#define MOB_ACTION_FLY 0x17
#define MOB_ACTION_FLY_HOVER 0x18

#define MOB_ACTION_STAND 0x19
#define MOB_ACTION_SIT 0x1A
#define MOB_ACTION_LIE 0x1B

#define MOB_ACTION_SPAWN 0x1C
#define MOB_ACTION_TELEPORT 0x1D
#define MOB_ACTION_LOAD 0x1E

#define MOB_ACTION_MINE 0x1F
#define MOB_ACTION_CRAFT 0x20
#define MOB_ACTION_GATHER 0x21
#define MOB_ACTION_FISH 0x22
#define MOB_ACTION_SHOP 0x23

#define MOB_ACTION_EMOTE 0xFF

#endif