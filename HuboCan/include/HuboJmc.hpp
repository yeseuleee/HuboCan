#ifndef HUBOJMC_HPP
#define HUBOJMC_HPP

extern "C" {
#include "hubo_info_c.h"
}

#include "CanDevice.hpp"
#include "HuboJoint.hpp"
#include <string>

const char hubo2plus_1ch_code[] = "H2P_1CH";
const char hubo2plus_2ch_code[] = "H2P_2CH";
const char hubo2plus_3ch_code[] = "H2P_3CH";
const char hubo2plus_5ch_code[] = "H2P_5CH";

const char drchubo_2ch_code[] = "DRC_2CH";
const char drchubo_hybrid_code[] = "DRC_HYBRID";


namespace HuboCan {

class HuboJmc
{
public:

    hubo_jmc_info_t info;
    HuboJointPtrArray joints;

    void addJoint(HuboJoint* joint);
    bool sortJoints(std::string& error_report);

protected:

    HuboJointPtrMap _tempJointMap;

};

typedef std::vector<HuboJmc*> HuboJmcPtrArray;

class Hubo2PlusBasicJmc : public HuboJmc
{
public:

protected:

};

class Hubo2Plus2chJmc : public Hubo2PlusBasicJmc
{
public:

protected:

};

class Hubo2Plus3chJmc : public Hubo2PlusBasicJmc
{
public:

protected:

};

class Hubo2Plus5chJmc : public Hubo2PlusBasicJmc
{
public:

protected:

};

class DrcHubo2chJmc : public Hubo2Plus2chJmc
{
public:

protected:

};

class DrcHuboHybridJmc : public Hubo2PlusBasicJmc
{
public:

protected:

};

} // namespace HuboCan

#endif // HUBOJMC_HPP
