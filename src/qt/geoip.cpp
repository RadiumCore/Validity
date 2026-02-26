// Copyright (c) 2026 The Validity developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "geoip.h"
#include <QStringList>

namespace GeoIP {

struct GeoRegion {
    double lat;
    double lon;
    const char *name;
};

// Approximate continent/region centroids
static const GeoRegion regions[] = {
    { 38.0, -97.0, "North America" },    // 0
    { -14.0, -51.0, "South America" },    // 1
    { 50.0, 10.0, "Europe" },             // 2
    { 25.0, 45.0, "Middle East" },        // 3
    { 10.0, 25.0, "Africa" },             // 4
    { 35.0, 105.0, "East Asia" },         // 5
    { 20.0, 78.0, "South Asia" },         // 6
    { -25.0, 134.0, "Oceania" },          // 7
    { 60.0, 90.0, "Russia/CIS" },         // 8
    { 5.0, 115.0, "Southeast Asia" },     // 9
};

static const int NUM_REGIONS = 10;

// First-octet to region index mapping (rough approximation based on IANA allocation)
static int octetToRegion(int firstOctet)
{
    // Major North America blocks
    if ((firstOctet >= 3 && firstOctet <= 9) ||
        (firstOctet >= 12 && firstOctet <= 19) ||
        (firstOctet >= 23 && firstOctet <= 24) ||
        (firstOctet >= 32 && firstOctet <= 35) ||
        (firstOctet >= 44 && firstOctet <= 48) ||
        (firstOctet >= 52 && firstOctet <= 56) ||
        (firstOctet >= 63 && firstOctet <= 76) ||
        (firstOctet >= 96 && firstOctet <= 99) ||
        (firstOctet >= 104 && firstOctet <= 108) ||
        (firstOctet >= 128 && firstOctet <= 135) ||
        (firstOctet >= 142 && firstOctet <= 148) ||
        (firstOctet >= 152 && firstOctet <= 155) ||
        (firstOctet >= 160 && firstOctet <= 168) ||
        (firstOctet >= 198 && firstOctet <= 199) ||
        (firstOctet >= 204 && firstOctet <= 209) ||
        (firstOctet >= 216 && firstOctet <= 223))
        return 0;

    // Major Europe blocks
    if ((firstOctet == 2) ||
        (firstOctet >= 25 && firstOctet <= 31) ||
        (firstOctet >= 36 && firstOctet <= 37) ||
        (firstOctet == 46) ||
        (firstOctet == 51) ||
        (firstOctet >= 57 && firstOctet <= 62) ||
        (firstOctet >= 77 && firstOctet <= 95) ||
        (firstOctet >= 109 && firstOctet <= 127) ||
        (firstOctet >= 136 && firstOctet <= 141) ||
        (firstOctet >= 149 && firstOctet <= 151) ||
        (firstOctet >= 176 && firstOctet <= 195) ||
        (firstOctet >= 212 && firstOctet <= 215))
        return 2;

    // East Asia
    if ((firstOctet == 1) ||
        (firstOctet == 14) ||
        (firstOctet == 27) ||
        (firstOctet == 39) ||
        (firstOctet == 42) ||
        (firstOctet == 49) ||
        (firstOctet == 58) ||
        (firstOctet >= 60 && firstOctet <= 61) ||
        (firstOctet >= 101 && firstOctet <= 103) ||
        (firstOctet == 106) ||
        (firstOctet >= 110 && firstOctet <= 126) ||
        (firstOctet == 133) ||
        (firstOctet == 150) ||
        (firstOctet == 153) ||
        (firstOctet == 163) ||
        (firstOctet == 175) ||
        (firstOctet >= 202 && firstOctet <= 203) ||
        (firstOctet >= 210 && firstOctet <= 211) ||
        (firstOctet >= 218 && firstOctet <= 222))
        return 5;

    // South America
    if ((firstOctet == 177) ||
        (firstOctet == 179) ||
        (firstOctet == 181) ||
        (firstOctet >= 186 && firstOctet <= 191) ||
        (firstOctet >= 200 && firstOctet <= 201))
        return 1;

    // Oceania
    if ((firstOctet == 116) ||
        (firstOctet >= 120 && firstOctet <= 122) ||
        (firstOctet == 124) ||
        (firstOctet == 144))
        return 7;

    // Africa
    if ((firstOctet == 41) ||
        (firstOctet == 102) ||
        (firstOctet == 105) ||
        (firstOctet == 154) ||
        (firstOctet == 156) ||
        (firstOctet >= 196 && firstOctet <= 197))
        return 4;

    // South Asia (India, etc.)
    if ((firstOctet == 43) ||
        (firstOctet == 59) ||
        (firstOctet == 111) ||
        (firstOctet == 115) ||
        (firstOctet == 117) ||
        (firstOctet == 164) ||
        (firstOctet == 182))
        return 6;

    // Default: Europe (most crypto nodes tend to be NA/EU)
    return 2;
}

QPair<double, double> lookup(const QString &ipAddress)
{
    // Strip port if present (handle both "1.2.3.4:8333" and "[::1]:8333")
    QString ip = ipAddress;
    if (ip.startsWith('[')) {
        // IPv6 - place at center
        return QPair<double,double>(0.0, 0.0);
    }
    if (ip.contains(':')) {
        ip = ip.section(':', 0, 0);
    }

    QStringList parts = ip.split('.');
    if (parts.size() != 4)
        return QPair<double,double>(0.0, 0.0);

    int firstOctet = parts[0].toInt();
    int regionIdx = octetToRegion(firstOctet);
    if (regionIdx < 0 || regionIdx >= NUM_REGIONS)
        regionIdx = 2; // fallback Europe

    // Add visual spread based on remaining octets so peers don't stack
    double latJitter = (parts[1].toInt() % 20 - 10) * 0.5;
    double lonJitter = (parts[2].toInt() % 20 - 10) * 0.5;

    return QPair<double,double>(
        regions[regionIdx].lat + latJitter,
        regions[regionIdx].lon + lonJitter
    );
}

QString regionName(const QString &ipAddress)
{
    QString ip = ipAddress;
    if (ip.startsWith('['))
        return "Unknown";
    if (ip.contains(':'))
        ip = ip.section(':', 0, 0);

    QStringList parts = ip.split('.');
    if (parts.size() != 4)
        return "Unknown";

    int regionIdx = octetToRegion(parts[0].toInt());
    if (regionIdx < 0 || regionIdx >= NUM_REGIONS)
        return "Unknown";

    return regions[regionIdx].name;
}

} // namespace GeoIP
