#include "SpacerFinder.h"

#include "Tools/UtilFunctions.h"
#include "Tools/Logs.h"

using std::string;

namespace TagsSearch
{
	const SpacerFinder::len_t SpacerFinder::ERR_CODE;

	SpacerFinder::SpacerFinder(const boost::property_tree::ptree &config, const std::string &reads_params_file)
		: spacer(config.get<string>("spacer"))
		, max_spacer_ed(config.get<unsigned>("max_spacer_edit_distance"))
		, spacer_min_pos(config.get<size_t>("barcode1_min_length"))
		, spacer_max_pos(config.get<size_t>("barcode1_max_length"))
		, barcode_length(config.get<size_t>("barcode2_length"))
		, umi_length(config.get<size_t>("umi_length"))
		, r1_rc_length(config.get<size_t>("r1_rc_length"))
	{
		auto spacer_prefix_length = config.get<size_t>("spacer_search_length");

		this->min_seq_len = this->spacer_min_pos + this->barcode_length + this->umi_length + this->spacer.length();

		if (this->spacer.length() <= spacer_prefix_length)
			throw std::runtime_error(
					"Spacers length must be larger than spacer_search_length (" +
							std::to_string(spacer_prefix_length) + "): '" + this->spacer + "'");

		if (this->max_spacer_ed >= spacer_prefix_length)
			throw std::runtime_error(
					"Max edit distance must be less than spacer_search_length (" +
					std::to_string(spacer_prefix_length) + "): " + std::to_string(this->max_spacer_ed));

		this->spacer_prefix = this->spacer.substr(0, spacer_prefix_length);
		this->spacer_suffix = this->spacer.substr(this->spacer.length() - spacer_prefix_length);

		this->spacer_max_suffix_start = this->spacer_max_pos + this->spacer.length() +
				this->max_spacer_ed - this->spacer_prefix.length();
		this->spacer_min_suffix_start = this->spacer_min_pos + this->spacer.length() - this->spacer_prefix.length();
		this->spacer_min_suffix_start -= std::min(this->spacer_min_suffix_start, this->max_spacer_ed);
	}

	SpacerFinder::spacer_pos_t SpacerFinder::find_spacer(const string &seq)
	{
		if (seq.length() < this->min_seq_len)
		{
			this->outcomes.inc(OutcomesCounter::SHORT_SEQ);
			return std::make_pair(SpacerFinder::ERR_CODE, SpacerFinder::ERR_CODE);
		}

		spacer_pos_t spacer_pos = std::make_pair(seq.find(this->spacer), (string::size_type)0);
		spacer_pos.second = spacer_pos.first + this->spacer.length();
		if (spacer_pos.first == SpacerFinder::ERR_CODE)
		{
			spacer_pos = this->find_spacer_partial(seq);
		}

		if (spacer_pos.first == SpacerFinder::ERR_CODE)
		{
			this->outcomes.inc(OutcomesCounter::NO_SPACER);
			return std::make_pair(SpacerFinder::ERR_CODE, SpacerFinder::ERR_CODE);
		}

		if (seq.length() < spacer_pos.second + this->barcode_length + this->umi_length)
		{
			this->outcomes.inc(OutcomesCounter::SHORT_SEQ);
			return std::make_pair(SpacerFinder::ERR_CODE, SpacerFinder::ERR_CODE);
		}

		this->outcomes.inc(OutcomesCounter::OK);
		return spacer_pos;
	}

	SpacerFinder::spacer_pos_t SpacerFinder::find_spacer_partial(const string &seq)
	{
		len_t suffix_pos = seq.rfind(this->spacer_suffix, this->spacer_max_suffix_start);
		len_t spacer_pos = suffix_pos - this->spacer.length() + this->spacer_suffix.length();

		if (suffix_pos == string::npos || suffix_pos < this->spacer_min_suffix_start)
		{
			spacer_pos = seq.find(this->spacer_prefix, this->spacer_min_pos);
			if (spacer_pos == string::npos || spacer_pos > this->spacer_max_pos)
				return std::make_pair(SpacerFinder::ERR_CODE, SpacerFinder::ERR_CODE);
		}

		int ed = Tools::edit_distance(this->spacer.c_str(), seq.substr(spacer_pos, this->spacer.length()).c_str(), true,
		                              this->max_spacer_ed);

		if (ed > this->max_spacer_ed)
			return std::make_pair(SpacerFinder::ERR_CODE, SpacerFinder::ERR_CODE);

		this->outcomes.inc(OutcomesCounter::SPACER_MODIFIED);
		return std::make_pair(spacer_pos, spacer_pos + this->spacer.length());
	}

	string SpacerFinder::parse_cell_barcode(const string &seq, len_t spacer_start, len_t spacer_end) const
	{
		string barcode = seq.substr(spacer_end, this->barcode_length);
		if (barcode.length() != this->barcode_length)
		{
			L_ERR << "Barcode is too short (required length: " << this->barcode_length << "): '" << barcode << "'";
		}

		return seq.substr(0, spacer_start) + barcode;
	}

	string SpacerFinder::parse_umi_barcode(const string &seq, len_t spacer_end) const
	{
		string res = seq.substr(spacer_end + this->barcode_length, this->umi_length);
		if (res.length() != this->umi_length)
		{
			L_ERR << "UMI is too short (required length: " << this->umi_length << "): '" << res << "'";
		}

		return res;
	}

	string SpacerFinder::parse_r1_rc(const string &seq, len_t spacer_end) const
	{
		return seq.substr(spacer_end + this->barcode_length + this->umi_length - this->r1_rc_length, this->r1_rc_length);
	}

	const OutcomesCounter &SpacerFinder::get_outcomes_counter() const
	{
		return this->outcomes;
	}

}
