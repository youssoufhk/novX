#pragma once

#include "MathTools.h"
#include "Instruments/InterestRate.h"

using Time = double;
using Value = double;

Value price(YieldCurve zcInstrument, Time t_dPricingDate = 0.) {

	std::vector<Time> maturities = zcInstrument.getMaturities();
	std::vector<Value> interest_rates = zcInstrument.getInterestRates();
	InterpolationType interpolation_method = zcInstrument.getInterpolationMethod();

	Value interest_rate = interpolate(t_dPricingDate, maturities, interest_rates, interpolation_method);

	return exp(-interest_rate * t_dPricingDate);
}
Value price(Swap swapInstrument, Time t_dPricingDate = 0.)
{
	std::unordered_map<std::string, Swap::Parameter>  parameters = swapInstrument.getParameters();
	long notional = std::get<long>(parameters["notional"]);
	Value swap_strike = std::get<double>(parameters["strike"]);
	SwapType swap_type = std::get<SwapType>(parameters["swap_type"]);
	YieldCurve zc_instrument = std::get<YieldCurve>(parameters["zero_coupon"]);
	YieldCurve forward_instrument = std::get<YieldCurve>(parameters["forward_curve"]);
	std::vector<Time> payment_dates = std::get<std::vector<Time>>(parameters["payment_dates"]);
	
	std::vector<Time> deltas(1);//(payment_dates.size());
	deltas[0] = payment_dates[1] - payment_dates[0];
	//std::adjacent_difference(payment_dates.begin(), payment_dates.end(), deltas);

	// overwriting the start_date if the pricing_date comes later
	auto itStartDate = std::lower_bound(
		payment_dates.begin(),
		payment_dates.end(),
		t_dPricingDate);
	if (itStartDate != payment_dates.end())
	{
		payment_dates.front() = *itStartDate;
	}

	// also need to consider the case where payments where already made
	std::vector<Time> vdPricingDates;

	std::copy_if(
		payment_dates.begin(),
		payment_dates.end(),
		std::back_inserter(vdPricingDates),
		[&](const auto& value) { return value >= t_dPricingDate; });
	// here we include the last payment date also (by putting '>=' instead of '>')
	// because the forward rate needs the Ti and Ti-1 values to compute.

	payment_dates = vdPricingDates;

	// compute the zero coupon prices
	std::vector<Value> vdZeroCouponPrice;

	// compute the zero coupon prices and forward curve prices
	std::transform(
		payment_dates.begin(),
		payment_dates.end(),
		std::back_inserter(vdZeroCouponPrice),
		[&](Time t) { return price(zc_instrument, t); });

	std::vector<Value> vdForwardPrice;
	std::transform(
		payment_dates.begin(),
		payment_dates.end(),
		std::back_inserter(vdForwardPrice),
		[&](Time t) { return price(forward_instrument, t); });
	
	// compute the forward rates
	std::vector<Value> vdForwardRates;
	std::transform(
		vdForwardPrice.begin(),
		vdForwardPrice.end() - 1,
		vdForwardPrice.begin() + 1,
		std::back_inserter(vdForwardRates),
		[&](const Value& P1, const Value& P2)
		{
			return (P1 / P2 - 1) / deltas.front();
		});


	// compute the vector to cumulate
	std::vector<Value> vdDiscountedCashFlow;
	std::transform(
		vdZeroCouponPrice.begin() + 1,
		vdZeroCouponPrice.end(),
		vdForwardRates.begin(),
		std::back_inserter(vdDiscountedCashFlow),
		[&](const Value& dZeroCoupon, const Value& dForwardRate)
		{
			return deltas.front() * dZeroCoupon * (dForwardRate - swap_strike);
		});


	// compute the annuity
	Value m_dAnnuity = std::accumulate(
		vdDiscountedCashFlow.begin(),
		vdDiscountedCashFlow.end(), 0.);

	return swap_type == PAYER ? notional * m_dAnnuity : -notional * m_dAnnuity;

}

template <class Instrument>
Value price(Instrument instrument, Time pricingDate = 0.)
{
	return price(instrument, pricingDate);
}

template <class Instrument>
std::vector<Value> priceVector(std::vector<Instrument> instruments, Time pricingDate = 0.)
{
	std::vector<Value> priceVect;
	for (auto const& instrument : instruments)
	{
		priceVect.push_back(price<Instrument>(instrument, pricingDate));
	}
	return priceVect;
}

template <class Instrument>
class Stripper
{
public:

	Stripper() {}
	Stripper(std::vector<Time> t_vdMaturities,
		std::vector<Value> t_vdInterestRates,
		std::function<std::vector<Instrument>(YieldCurve&)> t_instruments,
		InterpolationType t_interpolationMethod = InterpolationType::LINEAR_ON_Y)
		: m_vdMaturities(t_vdMaturities),
		m_vdInterestRates(t_vdInterestRates),
		m_instruments(t_instruments),
		m_interpolationMethod(t_interpolationMethod),
		m_ZeroCoupon(t_vdMaturities, t_vdInterestRates, t_interpolationMethod)
	{}

	std::vector<Value> evaluateInstruments()
	{
		std::vector<Instrument> swapInstruments = m_instruments(m_ZeroCoupon);
		std::vector<Value> swapPrices = priceVector<Instrument>(swapInstruments);
		
		return swapPrices;
	}

	void calibrate()
	{
		auto objectiveFunction = [&](std::vector<Value> t_vdInterestRates)
		{
			m_ZeroCoupon(m_vdMaturities, t_vdInterestRates);
			std::vector<Value> priceVector = evaluateInstruments();
			return priceVector;
		};

		multivariateNewtonRaphson<Value>(
			m_vdInterestRates,
			objectiveFunction);

	}

	YieldCurve getZeroCoupon()
	{
		m_ZeroCoupon(m_vdMaturities, m_vdInterestRates);
		return m_ZeroCoupon;
	}

private:

	std::vector<Time> m_vdMaturities{};
	std::vector<Value> m_vdInterestRates{};
	InterpolationType m_interpolationMethod;

	YieldCurve m_ZeroCoupon;
	std::function<std::vector<Instrument>(YieldCurve&)> m_instruments;

};
