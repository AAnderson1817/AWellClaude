#ifndef HUNTER_POSE_H
#define HUNTER_POSE_H

#include "inhabitants.h"
#include <math.h>

/* Pure presentation geometry, in original pixels, with Y up and Z forward.
   Both renderers consume the same rigid shoulders and fixed-length arm chain.
   A failed solve is an authoring error, never permission to stretch an arm. */
typedef struct { float x,y,z; } HunterJoint;
typedef struct { HunterJoint shoulder,elbow,hand; float reach; int reachable; } HunterArm;
#define HUNTER_ARM_LENGTH 4.2f

static inline float HunterPoseClamp(float v,float lo,float hi){return fminf(hi,fmaxf(lo,v));}
static inline int HunterPoseMoving(const HunterView *h){
    return h->state==HUNTER_APPROACH||h->state==HUNTER_CARRY||h->state==HUNTER_RETURN||h->state==HUNTER_TEND_APPROACH;
}
static inline int HunterPoseSeated(const HunterView *h){
    return h->state==HUNTER_SIT||h->state==HUNTER_WARM||h->state==HUNTER_WATCH||h->state==HUNTER_TEND;
}
static inline float HunterPoseRoll(const HunterView *h){
    return -h->facing*((HunterPoseSeated(h)||h->state==HUNTER_PLACE)?.12f:(h->state==HUNTER_TAKEN?0:.025f));
}
static inline float HunterPoseYaw(const HunterView *h){
    float cx=h->x+h->w*.5f;
    return HunterPoseMoving(h)?h->facing*.23f:HunterPoseClamp((h->lookX-cx)/(TS*2.f),-1,1)*.28f;
}
static inline HunterJoint HunterPosePoint(const HunterView *h,float x,float y,float z){
    float yaw=HunterPoseYaw(h),roll=HunterPoseRoll(h);
    float rx=x*cosf(yaw)+z*sinf(yaw),rz=-x*sinf(yaw)+z*cosf(yaw);
    return (HunterJoint){h->x+h->w*.5f+rx*cosf(roll)-y*sinf(roll),
        -(h->y+h->h)+rx*sinf(roll)+y*cosf(roll),rz-.8f};
}
static inline HunterArm HunterPoseArm(const HunterView *h,int side,float restingStoneAngle){
    HunterArm a={0};
    a.shoulder=HunterPosePoint(h,side*1.92f,h->h*.68f,1.12f);
    float turn=h->stoneTurn+(h->carriedItem>=0?restingStoneAngle:0);
    float grip=h->carriedItem>=0?1.8f:.85f;
    /* Fingers meet the upper sides of a low stone and cradle a high one from
       below. The half-pixel contact slide remains on its rounded surface. */
    float cup=h->carriedItem>=0?HunterPoseClamp((5.5f-(h->y+h->h-h->handY))*.25f,-.5f,.5f):-.5f;
    a.hand=(HunterJoint){h->handX+side*grip*cosf(turn),-h->handY+cup+side*grip*sinf(turn),
        h->carriedItem>=0?3.12f:1.76f};
    float dx=a.hand.x-a.shoulder.x,dy=a.hand.y-a.shoulder.y,dz=a.hand.z-a.shoulder.z;
    a.reach=sqrtf(dx*dx+dy*dy+dz*dz);
    a.reachable=a.reach>0.0001f&&a.reach<=2*HUNTER_ARM_LENGTH;
    if(!a.reachable)return a;
    dx/=a.reach;dy/=a.reach;dz/=a.reach;
    /* A continuous down/forward elbow plane keeps the two joints bent at rest.
       Side bias separates their silhouettes without changing either length. */
    float bx=side*.12f,by=-.68f,bz=.72f,dot=bx*dx+by*dy+bz*dz;
    bx-=dot*dx;by-=dot*dy;bz-=dot*dz;
    float bn=sqrtf(bx*bx+by*by+bz*bz);
    if(bn<.0001f){bx=-dy;by=dx;bz=0;bn=sqrtf(bx*bx+by*by);}
    float bend=sqrtf(fmaxf(0,HUNTER_ARM_LENGTH*HUNTER_ARM_LENGTH-a.reach*a.reach*.25f));
    a.elbow=(HunterJoint){(a.shoulder.x+a.hand.x)*.5f+bx*bend/bn,
        (a.shoulder.y+a.hand.y)*.5f+by*bend/bn,(a.shoulder.z+a.hand.z)*.5f+bz*bend/bn};
    return a;
}
#endif
