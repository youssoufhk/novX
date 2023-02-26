#pragma once

#include "Printers.h"
#include "Pricers.h"
#include "InputBBG.h"

int mainYieldCurve()
{
    std::chrono::steady_clock::time_point start = std::chrono::high_resolution_clock::now();

    static auto swapInstruments = [&](YieldCurve& myZC)
    {
        std::vector<Swap> mySwapVect;
        for (size_t i = 0; i < maturities.size(); i++)
        {
            Swap swap(SwapType::PAYER, notional, strikes[i], 0., 0., maturities[i], (int)(4 * maturities[i]), myZC);
            mySwapVect.push_back(swap);
        }
        return mySwapVect;
    };

    Stripper<Swap> myBootstrapp(maturities, initialRates, swapInstruments, LOGLINEAR_ON_EXP_X_TIMES_Y);
    std::vector<Value> swapPrices = myBootstrapp.evaluateInstruments();

    std::cout << "\nMy swap prices before bootstrapp: " << "\n";
    for (size_t i = 0; i < maturities.size(); i++)
    {
        std::cout << "Swap(" << maturities[i] << "Y, " << strikes[i] * 100 << "%)::price(0) = " << swapPrices[i] << "\n";
    }
    std::cout << "\n*******************************************************************************************\n";

    std::cout << "\n";
    std::chrono::steady_clock::time_point startC = std::chrono::high_resolution_clock::now();
    myBootstrapp.calibrate();
    std::chrono::steady_clock::time_point stop = std::chrono::high_resolution_clock::now();
    std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - startC);
    std::cout << "\n*******************************************************************************************\n";

    std::cout << "\nRunning the calibration only took " << duration.count() << " milliseconds." << "\n";
    YieldCurve myDiscountCurve = myBootstrapp.getZeroCoupon();
    std::cout << "\n*******************************************************************************************\n";

    swapPrices = myBootstrapp.evaluateInstruments();

    std::cout << "\nMy swap prices after bootstrapp: " << "\n";
    for (size_t i = 0; i < maturities.size(); i++)
    {
        std::cout << "Swap(" << maturities[i] << "Y, " << strikes[i] * 100 << "%)::price(0) = " << swapPrices[i] << "\n";
    }
    std::cout << "\n*******************************************************************************************\n";

    std::cout << "\nMy bootstrapped curve points: " << "\n";
    std::vector<Value> zcRates;
    std::transform(maturities.begin(), maturities.end(), std::back_inserter(zcRates),
        [&](Value const& maturity) { return -log(price(myDiscountCurve, maturity)) / maturity; });

    for (size_t i = 0; i < maturities.size(); i++)
    {
        std::cout << "ZeroCouponRate(0, " << maturities[i] << ") = " << zcRates[i] << "\n";
    }
    std::cout << "\n*******************************************************************************************\n";

    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::cout << "\nRunning the code up to here took " << duration.count() << " milliseconds." << "\n";
    std::cout << "\n*******************************************************************************************\n";

    std::cout << "\nAdding the MultiCurve pricer : " << "\n";
    std::cout << "\n*******************************************************************************************\n";

    int i = 0;
    double testStrike = 0.2;
    Swap swap1(SwapType::PAYER, notional, testStrike, 0., 0., maturities[i], (int)(4 * maturities[i]), myDiscountCurve);
    Swap swap2(SwapType::PAYER, notional, testStrike, 0., 0., maturities[i], (int)(4 * maturities[i]), myDiscountCurve, myDiscountCurve);
    std::cout << "\nSanity check: " << "\n";
    std::cout << "My swap 1 price: " << price(swap1) << "\n";
    std::cout << "My swap 2 price: " << price(swap2) << "\n";
    std::cout << "\n*******************************************************************************************\n";

    // At this point we already know P(0, T) for the discount curve
    static auto instruments = [&](YieldCurve& myFwdCurve)
    {
        std::vector<Swap> mySwapVect;
        for (size_t i = 0; i < fwdMaturities.size(); i++)
        {
            Swap swap(SwapType::PAYER, notional, fwdStrikes[i], 0., 0., fwdMaturities[i], (int)((4 + i) * fwdMaturities[i]), myDiscountCurve, myFwdCurve);
            mySwapVect.push_back(swap);
        }
        return mySwapVect;
    };

    Stripper<Swap> fwdCurveBootstrapp(fwdMaturities, fwdInitialRates, instruments, LOGLINEAR_ON_EXP_X_TIMES_Y);

    std::cout << "\nMy swap prices with the fwd curve before bootstrapp: " << "\n";
    std::vector<Value> fwdSwapPrices = fwdCurveBootstrapp.evaluateInstruments();

    for (size_t i = 0; i < fwdMaturities.size(); i++)
    {
        std::cout << "Tenor Basis Swap(" << 4 + i << " payments, " << fwdMaturities[i] << "Y, " << fwdStrikes[i] * 100 << "%)::price(0) = " << fwdSwapPrices[i] << "\n";
    }
    std::cout << "\n*******************************************************************************************\n";

    std::cout << "\n";
    startC = std::chrono::high_resolution_clock::now();
    fwdCurveBootstrapp.calibrate();
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - startC);
    std::cout << "\n*******************************************************************************************\n";

    std::cout << "\nRunning the calibration only took " << duration.count() << " milliseconds." << "\n";
    YieldCurve myFwdZC = fwdCurveBootstrapp.getZeroCoupon();
    std::cout << "\n*******************************************************************************************\n";

    std::cout << "\nMy swap prices with the fwd curve after bootstrapp: " << "\n";
    fwdSwapPrices = fwdCurveBootstrapp.evaluateInstruments();

    for (size_t i = 0; i < fwdMaturities.size(); i++)
    {
        std::cout << "Tenor Basis Swap(" << 4 + i << " payments, " << fwdMaturities[i] << "Y, " << fwdStrikes[i] * 100 << "%)::price(0) = " << fwdSwapPrices[i] << "\n";
    }
    std::cout << "\n*******************************************************************************************\n";

    std::cout << "\nMy bootstrapped curve points: " << "\n";
    std::vector<Value> fwdRates;
    std::transform(fwdMaturities.begin(), fwdMaturities.end(), std::back_inserter(fwdRates),
        [&](Value const& maturity) { return -log(price(myFwdZC, maturity)) / maturity; });

    for (size_t i = 0; i < fwdMaturities.size(); i++)
    {
        std::cout << "FwdCurveRate(0, " << fwdMaturities[i] << ") = " << fwdRates[i] << "\n";
    }
    std::cout << "\n*******************************************************************************************\n";

    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::cout << "\nRunning the code up to here took " << duration.count() << " milliseconds." << "\n";
    std::cout << "\n*******************************************************************************************\n";

    i = 0;
    Swap swap(SwapType::PAYER, notional, strikes[i], 0., 0., maturities[i], (int)(4 * maturities[i]), myDiscountCurve);
    Swap fwdSwap(SwapType::PAYER, notional, strikes[i], 0., 0., maturities[i], (int)(4 * maturities[i]), myDiscountCurve, myFwdZC);
    std::cout << "\nCheck spread: " << "\n";
    std::cout << "My swap price with risk-free curve: " << price(swap) << "\n";
    std::cout << "My swap price with risky curve: " << price(fwdSwap) << "\n";
    std::cout << "\n*******************************************************************************************\n";

    std::cout << "\n*******************************************************************************************\n";
    std::cout << "\n*******************************************************************************************\n";
    std::cout << "\nAdding the Bloomberg data : " << "\n";
    std::cout << "\n*******************************************************************************************\n";

    std::cout << "\nOIS discount : " << "\n";
    std::cout << "\n*******************************************************************************************\n";

    // At this point we already know P(0, T) for the discount curve
    static auto bbgOIS = [&](YieldCurve& myCurve)
    {
        std::vector<Swap> mySwapVect;
        for (size_t i = 0; i < maturitiesOIS.size(); i++)
        {
            int nbOfPayments = (int)(maturitiesOIS[i] > 1 ? maturitiesOIS[i] : 1);
            Swap swap(SwapType::PAYER, notional, strikesOIS[i], 0., 0., maturitiesOIS[i], nbOfPayments, myCurve);
            mySwapVect.push_back(swap); // doesn't like the .push_back() I don't know why
        }
        return mySwapVect;
    };

    Stripper<Swap> bootstrappOIS(maturitiesOIS, initialRatesOIS, bbgOIS, LOGLINEAR_ON_EXP_X_TIMES_Y);

    std::cout << "\nMy swap prices with the OIS curve before bootstrapp: " << "\n";
    std::vector<Value> oisSwapPrices = bootstrappOIS.evaluateInstruments();

    for (size_t i = 0; i < maturitiesOIS.size(); i++)
    {
        std::cout << "OIS Swap(" << maturitiesOIS[i] << "Y, " << strikesOIS[i] * 100 << "%)::price(0) = " << oisSwapPrices[i] << "\n";
    }
    std::cout << "\n*******************************************************************************************\n";

    std::cout << "\n";
    startC = std::chrono::high_resolution_clock::now();
    bootstrappOIS.calibrate();
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - startC);
    std::cout << "\n*******************************************************************************************\n";

    std::cout << "\nRunning the OIS calibration took " << duration.count() << " milliseconds." << "\n";
    YieldCurve myOIS = bootstrappOIS.getZeroCoupon();
    std::cout << "\n*******************************************************************************************\n";

    std::cout << "\nMy swap prices with the OIS curve after bootstrapp: " << "\n";
    oisSwapPrices = bootstrappOIS.evaluateInstruments();

    for (size_t i = 0; i < maturitiesOIS.size(); i++)
    {
        std::cout << "OIS Swap(" << maturitiesOIS[i] << "Y, " << strikesOIS[i] * 100 << "%)::price(0) = " << oisSwapPrices[i] << "\n";
    }
    std::cout << "\n*******************************************************************************************\n";

    std::cout << "\nMy bootstrapped curve points: " << "\n";
    std::vector<Value> oisPrices;
    std::transform(maturitiesOIS.begin(), maturitiesOIS.end(), std::back_inserter(oisPrices),
        [&](Value const& maturity) { return price(myOIS, maturity); });

    for (size_t i = 0; i < maturitiesOIS.size(); i++)
    {
        std::cout << "DiscountOIS(0, " << maturitiesOIS[i] << ") = " << oisPrices[i] << "\n";
    }
    std::cout << "\n*******************************************************************************************\n";

    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::cout << "\nRunning the code up to here took " << duration.count() << " milliseconds." << "\n";
    std::cout << "\n*******************************************************************************************\n";

    std::cout << "\nEUR3M curve : " << "\n";
    std::cout << "\n*******************************************************************************************\n";

    // At this point we already know P(0, T) for the discount curve
    static auto bbgEUR3M = [&](YieldCurve& myCurve)
    {
        std::vector<Swap> mySwapVect;
        for (size_t i = 0; i < maturitiesEUR3M.size(); i++)
        {
            int nbOfPayments = (int)(maturitiesEUR3M[i] > 1 ? 4 * maturitiesEUR3M[i] : 4);
            Swap swap(SwapType::PAYER, notional, strikesEUR3M[i], 0., 0., maturitiesEUR3M[i], nbOfPayments, myOIS, myCurve);
            mySwapVect.push_back(swap); // doesn't like the .push_back() I don't know why
        }
        return mySwapVect;
    };

    Stripper<Swap> bootstrappEUR3M(maturitiesEUR3M, initialRatesEUR3M, bbgEUR3M, LOGLINEAR_ON_EXP_X_TIMES_Y);


    std::cout << "\nMy swap prices with the EUR3M curve before bootstrapp: " << "\n";
    std::vector<Value> eur3mSwapPrices = bootstrappEUR3M.evaluateInstruments();
    for (size_t i = 0; i < maturitiesEUR3M.size(); i++)
    {
        std::cout << "EUR3M Swap(" << maturitiesEUR3M[i] << "Y, " << strikesEUR3M[i] * 100 << "%)::price(0) = " << eur3mSwapPrices[i] << "\n";
    }
    std::cout << "\n*******************************************************************************************\n";

    std::cout << "\n";
    startC = std::chrono::high_resolution_clock::now();
    bootstrappEUR3M.calibrate();
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - startC);
    std::cout << "\n*******************************************************************************************\n";

    std::cout << "\nRunning the EUR3M calibration took " << duration.count() << " milliseconds." << "\n";
    YieldCurve myEUR3M = bootstrappEUR3M.getZeroCoupon();
    std::cout << "\n*******************************************************************************************\n";

    std::cout << "\nMy swap prices with the EUR3M curve after bootstrapp: " << "\n";
    eur3mSwapPrices = bootstrappEUR3M.evaluateInstruments();

    for (size_t i = 0; i < maturitiesEUR3M.size(); i++)
    {
        std::cout << "EUR3M Swap(" << maturitiesEUR3M[i] << "Y, " << strikesEUR3M[i] * 100 << "%)::price(0) = " << eur3mSwapPrices[i] << "\n";
    }
    std::cout << "\n*******************************************************************************************\n";

    std::cout << "\nMy EUR3M bootstrapped curve points: " << "\n";
    std::vector<Value> eur3mPrices;
    std::transform(maturitiesEUR3M.begin(), maturitiesEUR3M.end(), std::back_inserter(eur3mPrices),
        [&](Value const& maturity) { return price(myEUR3M, maturity); });

    for (size_t i = 0; i < maturitiesEUR3M.size(); i++)
    {
        std::cout << "EUR3M(0, " << maturitiesOIS[i] << ") = " << eur3mPrices[i] << "\n";
    }
    std::cout << "\n*******************************************************************************************\n";

    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::cout << "\nRunning all the code took " << duration.count() << " milliseconds." << "\n";
    std::cout << "\n*******************************************************************************************\n";

    return 0;
}

YieldCurve oisCurve()
{
    std::chrono::steady_clock::time_point start = std::chrono::high_resolution_clock::now();

    std::cout << "\n*******************************************************************************************\n";
    std::cout << "\nLoading Bloomberg data..." << "\n";
    std::cout << "\n*******************************************************************************************\n";
    std::cout << "\nBoostrapping OIS discount curve: " << "\n";
    std::cout << "\n*******************************************************************************************\n";

    // At this point we already know P(0, T) for the discount curve
    static auto bbgOIS = [&](YieldCurve& myCurve)
    {
        std::vector<Swap> mySwapVect;
        for (size_t i = 0; i < maturitiesOIS.size(); i++)
        {
            int nbOfPayments = (int)(maturitiesOIS[i] > 1 ? maturitiesOIS[i] : 1);
            Swap swap(SwapType::PAYER, notional, strikesOIS[i], 0., 0., maturitiesOIS[i], nbOfPayments, myCurve);
            mySwapVect.push_back(swap);
        }
        return mySwapVect;
    };

    Stripper<Swap> bootstrappOIS(maturitiesOIS, initialRatesOIS, bbgOIS, LOGLINEAR_ON_EXP_X_TIMES_Y);

    std::vector<Value> swapPricesOIS = bootstrappOIS.evaluateInstruments();
    std::cout << "\nMy swap prices with the OIS curve before bootstrapp: " << "\n";
    for (size_t i = 0; i < maturitiesOIS.size(); i++)
    {
        std::cout << "OIS Swap(" << maturitiesOIS[i] << "Y, " << strikesOIS[i] * 100 << "%)::price(0) = " << swapPricesOIS[i] << "\n";
    }
    std::cout << "\n*******************************************************************************************\n";

    std::cout << "\n";
    std::chrono::steady_clock::time_point startC = std::chrono::high_resolution_clock::now();
    bootstrappOIS.calibrate();
    std::chrono::steady_clock::time_point stop = std::chrono::high_resolution_clock::now();
    std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - startC);
    std::cout << "\n*******************************************************************************************\n";

    std::cout << "\nRunning the OIS curve calibration took " << duration.count() << " milliseconds." << "\n";
    YieldCurve myOIS = bootstrappOIS.getZeroCoupon();
    std::cout << "\n*******************************************************************************************\n";

    swapPricesOIS = bootstrappOIS.evaluateInstruments();
    std::cout << "\nMy swap prices with the OIS curve after bootstrapp: " << "\n";
    for (size_t i = 0; i < maturitiesOIS.size(); i++)
    {
        std::cout << "OIS Swap(" << maturitiesOIS[i] << "Y, " << strikesOIS[i] * 100 << "%)::price(0) = " << swapPricesOIS[i] << "\n";
    }
    std::cout << "\n*******************************************************************************************\n";

    std::cout << "\nMy bootstrapped curve points: " << "\n";
    for (size_t i = 0; i < maturitiesOIS.size(); i++)
    {
        std::cout << "DiscountOIS(0, " << maturitiesOIS[i] << ") = " << price(myOIS, maturitiesOIS[i]) << "\n";
    }
    std::cout << "\n*******************************************************************************************\n";

    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::cout << "\nRunning all the code took " << duration.count() << " milliseconds." << "\n";
    std::cout << "\n*******************************************************************************************\n";

    return myOIS;
}