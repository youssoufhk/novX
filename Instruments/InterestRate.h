#pragma once

using Time = double;
using Value = double;

enum SwapType
{
	PAYER = -1,
	RECEIVER = 1
};

class YieldCurve
{
public:
	YieldCurve() {}
	YieldCurve(
		std::vector<Time> t_vdMaturities,
		std::vector<Value> t_vdInterestRates,
		InterpolationType t_interpolationMethod = InterpolationType::LINEAR_ON_Y)
		: m_vdMaturities(t_vdMaturities),
		m_vdInterestRates(t_vdInterestRates),
		m_interpolationMethod(t_interpolationMethod)
	{}

	auto operator() (
		std::vector<Time> t_vdMaturities,
		std::vector<Value> t_vdInterestRates,
		InterpolationType t_interpolationMethod = InterpolationType::LINEAR_ON_Y)
	{
		m_vdMaturities = t_vdMaturities;
		m_vdInterestRates = t_vdInterestRates;
		m_interpolationMethod = t_interpolationMethod;
	}

	std::vector<Time> getMaturities()
	{
		return m_vdMaturities;
	}
	std::vector<Value> getInterestRates()
	{
		return m_vdInterestRates;
	}
	InterpolationType getInterpolationMethod()
	{
		return m_interpolationMethod;
	}
private:
	std::vector<Time> m_vdMaturities;
	std::vector<Value> m_vdInterestRates;
	InterpolationType m_interpolationMethod;
};

class Swap
{
public:
	Swap() {}
	Swap(SwapType t_SwapType,
		long t_Notional,
		Value t_Strike,
		Time t_PricingDate,
		Time t_StartDate,
		Time t_EndDate,
		size_t t_NbPayments,
		YieldCurve t_ZeroCoupon)
		:
		m_SwapType(t_SwapType),
		m_iNotional(t_Notional),
		m_dStrike(t_Strike),
		m_dPricingDate(t_PricingDate),
		m_dStartDate(t_StartDate),
		m_dEndDate(t_EndDate),
		m_dNbPayments(t_NbPayments),
		m_vdPaymentDates(linspace<Time>(m_dStartDate, m_dEndDate, m_dNbPayments)),
		m_ZeroCoupon(t_ZeroCoupon),
		m_ForwardCurve(m_ZeroCoupon)
	{}

	Swap(SwapType t_SwapType,
		long t_Notional,
		Value t_Strike,
		Time t_PricingDate,
		Time t_StartDate,
		Time t_EndDate,
		size_t t_NbPayments,
		YieldCurve t_ZeroCoupon,
		YieldCurve t_ForwardCurve)
		:
		m_SwapType(t_SwapType),
		m_iNotional(t_Notional),
		m_dStrike(t_Strike),
		m_dPricingDate(t_PricingDate),
		m_dStartDate(t_StartDate),
		m_dEndDate(t_EndDate),
		m_dNbPayments(t_NbPayments),
		m_vdPaymentDates(linspace<Time>(m_dStartDate, m_dEndDate, m_dNbPayments)),
		m_ZeroCoupon(t_ZeroCoupon),
		m_ForwardCurve(t_ForwardCurve)
	{}

	using Parameter = std::variant<
		int,
		long,
		double,
		size_t,
		SwapType,
		YieldCurve,
		std::vector<Time>>;

	std::unordered_map<std::string, Parameter> getParameters()
	{
		std::unordered_map<std::string, Parameter> myMap;
		myMap["swap_type"] = m_SwapType;
		myMap["notional"] = m_iNotional;
		myMap["strike"] = m_dStrike;
		myMap["pricing_date"] = m_dPricingDate;
		myMap["zero_coupon"] = m_ZeroCoupon;
		myMap["forward_curve"] = m_ForwardCurve;
		myMap["payment_dates"] = m_vdPaymentDates;
		return myMap;
	}

private:
	SwapType m_SwapType;
	long m_iNotional;
	Value m_dStrike;
	Time m_dPricingDate;
	Time m_dStartDate;
	Time m_dEndDate;
	size_t m_dNbPayments;
	YieldCurve m_ZeroCoupon;
	YieldCurve m_ForwardCurve;

	std::vector<Time> m_vdPaymentDates;
};
