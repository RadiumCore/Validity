// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_PARAMS_H
#define BITCOIN_CONSENSUS_PARAMS_H

#include "uint256.h"
#include <map>
#include <string>

namespace Consensus {

enum DeploymentPos
{
    DEPLOYMENT_TESTDUMMY,
    DEPLOYMENT_CSV, // Deployment of BIP68, BIP112, and BIP113.
    // NOTE: Also add new deployments to VersionBitsDeploymentInfo in versionbits.cpp
    MAX_VERSION_BITS_DEPLOYMENTS
};

/**
 * Struct for each individual consensus rule change using BIP9.
 */
struct BIP9Deployment {
    /** Bit position to select the particular bit in nVersion. */
    int bit;
    /** Start MedianTime for version bits miner confirmation. Can be a date in the past */
    int64_t nStartTime;
    /** Timeout/expiry MedianTime for the deployment attempt. */
    int64_t nTimeout;
};

/**
 * Parameters that influence chain consensus.
 */
struct Params {
    uint256 hashGenesisBlock;
    int nMaxReorganizationDepth;
    /** Used to check majorities for block version upgrade */
    int nMajorityEnforceBlockUpgrade;
    int nMajorityRejectBlockOutdated;
    int nMajorityWindow;
    /** Block height and hash at which BIP34 becomes active */
    int BIP34Height;
    uint256 BIP34Hash;
    /**
     * Minimum blocks including miner confirmation of the total of 2016 blocks in a retargetting period,
     * (nTargetTimespan / nTargetSpacing) which is also used for BIP9 deployments.
     * Examples: 1916 for 95%, 1512 for testchains.
     */
    uint32_t nRuleChangeActivationThreshold;
    uint32_t nMinerConfirmationWindow;
    BIP9Deployment vDeployments[MAX_VERSION_BITS_DEPLOYMENTS];
    /** Proof of work parameters */
    uint256 powLimit;
    uint256 posLimit;
    uint256 posLimitV2;
    bool fPowAllowMinDifficultyBlocks;
    int64_t nTargetSpacingV1;
    bool fPowNoRetargeting;
    bool fPoSNoRetargeting;
    int64_t nTargetSpacing;
    int64_t nTargetTimespan;
    int64_t nTargetTimespanNEW; // 15 min
    int64_t DifficultyAdjustmentInterval() const { return nTargetTimespan / nTargetSpacing; }
    int64_t nProtocolV1RetargetingFixedTime;
    int64_t nGenesisBlockTime;
    int64_t nProtocolV2Time;
    int64_t nProtocolV3Time;
    int64_t AvgFeeProtocolTime;
    int DEV_FUND_BLOCK_HEIGHT ; // Developers Fund block height
    int64_t DEV_FUND_TIME ;
    std::string DEV_FUND_SCRIPT ;
    int AVG_FEE_START_BLOCK;
    int AVG_FEE_START_BLOCK_REVERT;
    int AVG_FEE_START_BLOCK_V2;
    int AVG_FEE_SPAN;
    //const char* DEV_FUND_SCRIPT = x.c_str(); // This is a 5-of-12 multisig address for radium development fund address: QVZ419DruuEQYxbCgvF6vNwYTJizbhC9qw



    
    bool IsProtocolV1RetargetingFixed(int64_t nTime) const { return nTime > nGenesisBlockTime; }
    bool IsProtocolV2(int64_t nTime) const { return nTime > nGenesisBlockTime; }
    bool IsProtocolV3(int64_t nTime) const { return nTime > nProtocolV3Time; }
    bool IsAvgFeeProtocol(int64_t nTime) const { return nTime > AvgFeeProtocolTime; }
    bool IsProtocolV4(int nHeight) const {   return nHeight >= DEV_FUND_BLOCK_HEIGHT;    }
    bool IsProtocolV4(int64_t nTime) const { return nTime >= DEV_FUND_TIME; }
    bool IsBlockDevFund(int nHeight) const
    {
        // first check if we are past dev fund start heigt
        if (IsProtocolV4(nHeight)) {
            // check if it is time to pay out (~ weekly, )

            if (nHeight % 10080 == 0) {               
                return true;
            }
           
        }
        return false;
    }  

	
	inline int64_t FutureDriftV1(int64_t nTime) const { return nTime + 10 * 60; }
    inline int64_t FutureDriftV2(int64_t nTime) const { return nTime + 2 * 60; }
    inline int64_t FutureDrift(int64_t nTime) const { return IsProtocolV2(nTime) ? FutureDriftV2(nTime) : FutureDriftV1(nTime); }

   
    int64_t GetTargetSpacing(int nHeight) const { return IsProtocolV4(nHeight) ? 57 : 60; }
    int nLastPOWBlock;
    int nStakeTimestampMask;
    int nCoinbaseMaturity;
    static const unsigned int nStakeMinConfirmationsTest = 10;
    static const unsigned int nStakeMinConfirmationsV3 = 60;
    static const unsigned int nStakeMinConfirmationsV4 = 120;
    int nStakeMaturity(int nHeight) const
    {
        if (IsProtocolV4(nHeight))
            return nStakeMinConfirmationsV4;
        else
            return nStakeMinConfirmationsV3;
    }
    int nStakeMaturity_Time(int64_t nTime) const
    {
        if (IsProtocolV4(nTime))
            return nStakeMinConfirmationsV4;
        else
            return nStakeMinConfirmationsV3;
    }
    unsigned int nStakeMinAge;
    uint256 nMinimumChainWork;

	


};
} // namespace Consensus

#endif // BITCOIN_CONSENSUS_PARAMS_H
