//*****************************************************************************
//*****************************************************************************

#include "xbridgewalletconnector.h"
#include "xbridgetransactiondescr.h"
#include "base58.h"

//*****************************************************************************
//*****************************************************************************
namespace xbridge
{

//*****************************************************************************
//*****************************************************************************
namespace wallet
{

//*****************************************************************************
//*****************************************************************************
std::string UtxoEntry::toString() const
{
    std::ostringstream o;
    o << txId << ":" << vout << ":" << amount << ":" << address;
    return o.str();
}

} // namespace wallet

//*****************************************************************************
//*****************************************************************************
WalletConnector::WalletConnector()
{
}

//******************************************************************************
//******************************************************************************

/**
 * \brief Return the wallet balance; optionally for the specified address.
 * \param addr Optional address to filter balance
 * \return returns the wallet balance for the address.
 *
 * The wallet balance for the specified address will be returned. Only utxo's associated with the address
 * are included.
 */
double WalletConnector::getWalletBalance(const std::string & addr) const
{
    std::vector<wallet::UtxoEntry> entries;
    if (!getUnspent(entries))
    {
        LOG() << "getUnspent failed " << __FUNCTION__;
        return -1.;//return negative value for check in called methods
    }

    double amount = 0;
    for (const wallet::UtxoEntry & entry : entries)
    {
        // exclude utxo's not matching address
        if (!addr.empty() && entry.address != addr)
        {
            continue;
        }
        amount += entry.amount;
    }

    return amount;
}

//******************************************************************************
//******************************************************************************

/**
 * \brief Checks if specified address has a valid prefix.
 * \param addr Address to check
 * \return returns true if address has a valid prefix, otherwise false.
 *
 * If the specified wallet address has a valid prefix the method returns true, otherwise false.
 */
bool WalletConnector::hasValidAddressPrefix(const std::string & addr) const
{
    std::vector<unsigned char> decoded;
    if (!DecodeBase58Check(addr, decoded))
    {
        return false;
    }

    bool isP2PKH = memcmp(addrPrefix,   &decoded[0], decoded.size()-sizeof(uint160)) == 0;
    bool isP2SH  = memcmp(scriptPrefix, &decoded[0], decoded.size()-sizeof(uint160)) == 0;

    return isP2PKH || isP2SH;
}

//******************************************************************************
//******************************************************************************
bool WalletConnector::lockCoins(const std::vector<wallet::UtxoEntry> & inputs,
                                const bool lock)
{
    std::lock_guard<std::mutex> l(lockedCoinsLocker);

    if (!lock)
    {
        for (const wallet::UtxoEntry & entry : inputs)
        {
            lockedCoins.erase(entry);
        }
    }
    else
    {
        // check duplicates
        for (const wallet::UtxoEntry & entry : inputs)
        {
            if (lockedCoins.count(entry))
            {
                return false;
            }
        }

        lockedCoins.insert(inputs.begin(), inputs.end());
    }

    return true;
}

//******************************************************************************
//******************************************************************************
void WalletConnector::removeLocked(std::vector<wallet::UtxoEntry> & inputs) const
{
    std::lock_guard<std::mutex> lock(lockedCoinsLocker);

    for (auto it = inputs.begin(); it != inputs.end(); )
    {
        if (lockedCoins.count(*it))
        {
            it = inputs.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

} // namespace xbridge
