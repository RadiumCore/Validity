// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/merkle.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

#include "chainparamsseeds.h"

using namespace std;

static CBlock CreateGenesisBlock(const char* pszTimestamp, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    // Genesis block

    // MainNet:

    //CBlock(hash=000001faef25dec4fbcf906e6242621df2c183bf232f263d0ba5b101911e4563, ver=1, hashPrevBlock=0000000000000000000000000000000000000000000000000000000000000000, hashMerkleRoot=12630d16a97f24b287c8c2594dda5fb98c9e6c70fc61d44191931ea2aa08dc90, nTime=1393221600, nBits=1e0fffff, nNonce=164482, vtx=1, vchBlockSig=)
    //  Coinbase(hash=12630d16a9, nTime=1393221600, ver=1, vin.size=1, vout.size=1, nLockTime=0)
    //    CTxIn(COutPoint(0000000000, 4294967295), coinbase 00012a24323020466562203230313420426974636f696e2041544d7320636f6d6520746f20555341)
    //    CTxOut(empty)
    //  vMerkleTree: 12630d16a9

    // TestNet:

    //CBlock(hash=0000724595fb3b9609d441cbfb9577615c292abf07d996d3edabc48de843642d, ver=1, hashPrevBlock=0000000000000000000000000000000000000000000000000000000000000000, hashMerkleRoot=12630d16a97f24b287c8c2594dda5fb98c9e6c70fc61d44191931ea2aa08dc90, nTime=1393221600, nBits=1f00ffff, nNonce=216178, vtx=1, vchBlockSig=)
    //  Coinbase(hash=12630d16a9, nTime=1393221600, ver=1, vin.size=1, vout.size=1, nLockTime=0)
    //    CTxIn(COutPoint(0000000000, 4294967295), coinbase 00012a24323020466562203230313420426974636f696e2041544d7320636f6d6520746f20555341)
    //    CTxOut(empty)
    //  vMerkleTree: 12630d16a9

    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.nTime = nTime;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 0 << CScriptNum(42) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(txNew);
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "2015 xRadon coin.!";
   
    return CreateGenesisBlock(pszTimestamp, nTime, nNonce, nBits, nVersion, genesisReward);
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */

class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        consensus.nMaxReorganizationDepth = 500;
        consensus.nMajorityEnforceBlockUpgrade = 750;
        consensus.nMajorityRejectBlockOutdated = 950;
        consensus.nMajorityWindow = 1000;
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.posLimit = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.posLimitV2 = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nTargetTimespan = 1 * 60; // 1 mins
        consensus.nTargetTimespanNEW = 15 * 60; // 15 mins
        consensus.nTargetSpacingV1 = 60;
        consensus.nTargetSpacing = 64;
        consensus.BIP34Height = -1;
        consensus.BIP34Hash = uint256();
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.fPoSNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nTargetTimespan / nTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        /*
        Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 999999999999ULL; // never
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 0; // out of time
        */

        consensus.nProtocolV1RetargetingFixedTime = 1395631999;
        consensus.nProtocolV2Time = 1407053625;
        consensus.nProtocolV3Time = 1461851161;
        consensus.nLastPOWBlock = 20160;
        consensus.nStakeTimestampMask = 0xf; // 15
        consensus.nCoinbaseMaturity = 60;
        consensus.nStakeMinAge = 6 * 60 * 60; // 6 hours

        // The best chain should have at least this much work.
		consensus.nMinimumChainWork = uint256S("0x000000000000000000000000000000000000000000000036923aefeb05b43b54");

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0x2a;
        pchMessageStart[1] = 0x7c;
        pchMessageStart[2] = 0xcb;
        pchMessageStart[3] = 0xab;
        nDefaultPort = 27913;
        nPruneAfterHeight = 100000;

        genesis = CreateGenesisBlock(1431857735, 109996, 0x1e0fffff, 1, 0);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x000000770c6aea829bb1ace7b06497f71799a6358e0e292740c4f9443a17bfb6"));
        assert(genesis.hashMerkleRoot == uint256S("0xa02b3388d9e8529bb0136ed3e1b823e808cbe128050e5986bf2a5a0d2c71a826"));

        

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,76);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,58);
        base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1,121);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x73)(0xAA)(0xA1).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x73)(0x44)(0x77).convert_to_container<std::vector<unsigned char> >();
        cashaddrPrefix = "validity";

		vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = false;
		
        checkpointData = (CCheckpointData) {
                    boost::assign::map_list_of
					(0, uint256S("0x000000770c6aea829bb1ace7b06497f71799a6358e0e292740c4f9443a17bfb6")) //genesis
			(2879, uint256S("0x675ebb9f0f934c35bbf1955565482601ef68977b2fef8248689626004fcbce2d"))      //last pow    
                        (2880, uint256S("0x3818386bc9fced59f21d5a9d1eab8a532c85efecb831eceee995ae1a7dc90419"))      //first pos   
                        (100000, uint256S("0x97b8764441d1d0eff98e834cc743ec4cd6779d53f149dec9403c9758370abd8c"))
			(200000, uint256S("0x98b0bcf0218293549a30d938a1d641b6bc6b498361cb432e3fafd6935c592846"))
			(300000, uint256S("0xdcb25c38e2731a68d5bfa16b00b8479fed57c6a352cfd94d8e0dfdd690ac9ea2"))
			(440000, uint256S("0x88914855018cf647d1b20162eed2dbc7cdf69757f8216868c1b55687aafe1a75"))
			(500000, uint256S("0x41950e8d38381ce56fc0fd9eca76e73b49ce256c01ab2521897c5c6285b39c8d"))
			(550000, uint256S("0x8a39262d43135792a429a4e2a6deeb669c88b618ecfa3ca855edaf1e4c3631af"))
			(600000, uint256S("0x23141467be3b6fe23419c8a9fbcd4df431b4b1abcd43a1fa689e7b6d137e89ef"))
			(619480, uint256S("0x17895abf56169141d553891d00a4c46d37023250a0089800e389bfba37e91f9a")) // avg_fee start block 
                        (625249, uint256S("0x677def44b35e162452fb12f00c4e4fa5f1339a90a4b82753266e1cd9f5b359e7")) // problem block on avg_fee    
                        (626372, uint256S("0x977494d951029df80bee05bdd461e6e767c916c2b60ebd356133c38ae5cf5823")) // pre avg_fee revert block    
                        (626555, uint256S("0xab4e8bdd76283070e11377f9497d7a644d9a813fc354152b36ded76d68eb5388")) // post avg_fee revert block  
                        (655200, uint256S("0x0dbcd2799042832cb3c1c0255e34b1ca797d30db9438d836e9e830394e98b999 ")) // Radium 1.4.5 Code release    
                        (700000, uint256S("0x6ceebeff2760988638367e5e91a9061bdc9623db1dceeaa7cef1eb8da88ec2bd "))
			(800000, uint256S("0xbce932a50e264133d5456f1a8076552f5186f3f6ff46310f4c769ae83c4a1723 "))
			(900000, uint256S("0x5c58c0b472ab44dbde75c31e549615fc00feea3a93844d8c78837b2e85245c4e "))
			(950000, uint256S("0x75ece61af3a0bf5243ff0a7773a8f2bb560f52ba6aa2652f0de1ce3c1199ede8"))
			(1000000, uint256S("0x6950e1ca2bef4adfb963df7edaf92cac8127d3445554d23738bd88c135ce59e4"))
			(1050000, uint256S("0x104c2d6ffa918160fb3878f89323920d037f857854782c0436a5df4be42e0492"))
			(1100000, uint256S("0x0978eceb49ed5dc681c766b5cba395b5008d8dbadba81f37e86fdeff61fc38d5"))
			(1150000, uint256S("0x12c172f98f5c491f5f230bef5040a50c4ddf8526c5160b419a708564dd1ec0c9"))
			(1200000, uint256S("0x619ffe5e099a182afaf5c37b0c73e60fd858acb2231ee29d95f4a223ba6e429a"))
			(1250000, uint256S("0x12cd34ed7b24ecf5414fdf175b72fbf30db043cf51b53d4efa79a5272e91ab9d"))
			(1300000, uint256S("0x493d0c754b68a09f0c22bea134264aceebc8acc13c02a80bd57632876b54e2f6"))
			(1350000, uint256S("0x6bd41114d13dc0e02a426c92cdbc90e6c852c36250b1a91d1705cc7b360efd3c"))
			(1400000, uint256S("0x77d89a7da8d763c231ce82d8ea985648933446e952aadc19d8488c6b1813b1d7"))
			(1450000, uint256S("0x7cce29249398313d77075c1681d08949e3409d318a293b0c890a11cba2593c0d"))
			(1500000, uint256S("0xe01b2489defc9e272cf455718774341809c04ab97733a5c12308c9e9e9dda031"))
			(1550000, uint256S("0x152f86b58e2f86467b1770e2ba2fa53d72f8f29f0c9ba19327a5b9f403358904"))
			(1594200, uint256S("0x00062974544477352134e8396c2186610bf744998b2f34cf2005eacaccd2cffb"))
			(1600000, uint256S("0xff5404d458f342ce6bee860e7c03ec847943da161cdb9914c23d5401d1744908"))
			(1700000, uint256S("0x3191191349cecf60db3da8bb467f01d05c85f3d8cdd4f921060dda5be31cddd1"))
			(1800000, uint256S("0x8cb0ad88427f7f98b66d1018bcba9e78a873c89fcdefa4c87c33e56aeaba2dde"))
			(1900000, uint256S("0x95a2b47511627b57c4307f19fc45ba52a34446d68100c55bdaca1eb0f1b63bcb"))
			(2000000, uint256S("0x6f5ffb06006b6724caf38de9fa67c8c98c3cd87ab0aca9a84cb983dc858be901"))
			(2100000, uint256S("0x483f9d89fa11bee453f684eef918f1eb06e7ca2f0672f45c9fd7dd9d8e8fa995"))
			(2200000, uint256S("0x231e74358bb0b0dc6b1261d16935d45a10982bf66c0c7ab208f02e28b9b6ae61"))
			(2300000, uint256S("0x462e12a20897f054e182f83c20ee30d5c72755542fae6729afaa9ecdef714c9e"))
			(2400000, uint256S("0xb670ba23f814d23ec129999b26bb734011c06f51cf5887ae871a9500fd7158ef"))
			(2500000, uint256S("0xef8c91c73bc9c17aa479beab954049500f460d7a4a63549e51e58eb423b17b46"))
			(2500000, uint256S("0xef8c91c73bc9c17aa479beab954049500f460d7a4a63549e51e58eb423b17b46"))
			(2600000, uint256S("0x3f9e88853f237ecaad98b4f117cdae00e111ecc7aae2191a7a8ae4b4e2b43001"))
			(2700000, uint256S("0xc9167aa8f71e3344f7e5829dda96311996b6527c5b76b0147f534f199b9e9727"))
			(2800000, uint256S("0x0c565de4d82c333d827cb1058193dd8d4027d390deeb575b471783191a219122"))
			(2893756, uint256S("0xf14befb066eb08792fd480fa52243654f47b19d5894e198f951eacd819e46a85"))
			    ,
		
			1609206848, // * UNIX timestamp of last checkpoint block
			5871254,    // * total number of transactions between genesis and last checkpoint
								//   (the tx=... number in the SetBestChain debug.log lines)
			10.0      // * estimated number of transactions per day after checkpoint			
        };
    }
};
static CMainParams mainParams;

/**
 * Testnet
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.nMaxReorganizationDepth = 500;
        consensus.nMajorityEnforceBlockUpgrade = 750;
        consensus.nMajorityRejectBlockOutdated = 950;
        consensus.nMajorityWindow = 1000;
        consensus.powLimit = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.posLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.posLimitV2 = uint256S("000000000000ffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nTargetTimespan = 16 * 60; // 16 mins
        consensus.nTargetSpacingV1 = 60;
        consensus.nTargetSpacing = 64;
        consensus.BIP34Height = -1;
        consensus.BIP34Hash = uint256();
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.fPoSNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        consensus.nProtocolV1RetargetingFixedTime = 1395631999;
        consensus.nProtocolV2Time = 1407053625;
        consensus.nProtocolV3Time = 1444028400;
        consensus.nLastPOWBlock = 0x7fffffff;
        consensus.nStakeTimestampMask = 0xf;
        consensus.nCoinbaseMaturity = 10;
        consensus.nStakeMinAge = 8 * 60 * 60;

        pchMessageStart[0] = 0xcd;
        pchMessageStart[1] = 0xf2;
        pchMessageStart[2] = 0xc0;
        pchMessageStart[3] = 0xef;
        nDefaultPort = 25714;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        /*
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 999999999999ULL; // never
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 0; // out of time
        */

        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1393221600, 216178, 0x1f00ffff, 1, 0);
        consensus.hashGenesisBlock = genesis.GetHash();
        //assert(consensus.hashGenesisBlock == uint256S("0x0000724595fb3b9609d441cbfb9577615c292abf07d996d3edabc48de843642d"));
        //assert(genesis.hashMerkleRoot == uint256S("0x12630d16a97f24b287c8c2594dda5fb98c9e6c70fc61d44191931ea2aa08dc90"));

        vFixedSeeds.clear();
        vSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();
        cashaddrPrefix = "blktest";

        

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = true;

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            ( 90235, uint256S("0x567898e79184dc2f7dc3a661f794f28566e4b856d70180914f7371b1b3cc82d8")),
            1549558800,
            179080,
            2.0
        };

    }
};
static CTestNetParams testNetParams;

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.nMaxReorganizationDepth = 50;
        consensus.nMajorityEnforceBlockUpgrade = 51;
        consensus.nMajorityRejectBlockOutdated = 75;
        consensus.nMajorityWindow = 100;
        consensus.powLimit = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.posLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.posLimitV2 = uint256S("000000000000ffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nTargetTimespan = 16 * 60; // 16 mins
        consensus.nTargetSpacingV1 = 64;
        consensus.nTargetSpacing = 64;
        consensus.BIP34Height = -1; // BIP34 has not necessarily activated on regtest
        consensus.BIP34Hash = uint256();
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.fPoSNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108;// 75% for regtest
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        /*
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 999999999999ULL; // never
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 0; // out of time
        */

        consensus.nProtocolV1RetargetingFixedTime = 1395631999;
        consensus.nProtocolV2Time = 1407053625;
        consensus.nProtocolV3Time = 1444028400;
        consensus.nLastPOWBlock = 1000;
        consensus.nStakeTimestampMask = 0xf;
        consensus.nCoinbaseMaturity = 10;
        consensus.nStakeMinAge = 1 * 60 * 60;

        pchMessageStart[0] = 0x70;
        pchMessageStart[1] = 0x35;
        pchMessageStart[2] = 0x22;
        pchMessageStart[3] = 0x06;
        nDefaultPort = 25714;
        nPruneAfterHeight = 100000;

        genesis = CreateGenesisBlock(1393221600, 216178, 0x1f00ffff, 1, 0);
        consensus.hashGenesisBlock = genesis.GetHash();
        //assert(consensus.hashGenesisBlock == uint256S("0x0000724595fb3b9609d441cbfb9577615c292abf07d996d3edabc48de843642d"));
       // assert(genesis.hashMerkleRoot == uint256S("0x12630d16a97f24b287c8c2594dda5fb98c9e6c70fc61d44191931ea2aa08dc90"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();
        cashaddrPrefix = "blkreg";

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fTestnetToBeDeprecatedFieldRPC = false;

    }

    void UpdateBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
    {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
    }
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = 0;

const CChainParams &Params() {
    assert(pCurrentParams);
    return *pCurrentParams;
}

CChainParams& Params(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
            return mainParams;
    else if (chain == CBaseChainParams::TESTNET)
            return testNetParams;
    else if (chain == CBaseChainParams::REGTEST)
            return regTestParams;
    else
        throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    pCurrentParams = &Params(network);
}

void UpdateRegtestBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    regTestParams.UpdateBIP9Parameters(d, nStartTime, nTimeout);
}
