#include "Cell.h"

namespace Estimation
{
	Cell::Cell(const std::string &barcode, size_t min_genes_to_be_real, const std::vector<UMI::Mark> &query_marks)
			: _barcode(barcode)
			, _min_genes_to_be_real(min_genes_to_be_real)
			, _query_marks(query_marks)
			, _is_merged(false)
			, _is_excluded(false)
			, _umis_number(0)
			, _requested_genes_num(0)
	{}

	void Cell::add_umi(const std::string &gene, const std::string &umi, const UMI::Mark &umi_mark)
	{
		bool is_new = this->_genes[gene].add_umi(umi, umi_mark);
		if (is_new)
		{
			this->_umis_number++;
		}
	}

	void Cell::set_merged()
	{
		this->_is_merged = true;
	}

	void Cell::set_excluded()
	{
		this->_is_excluded = true;
	}

	void Cell::merge(const Cell &source)
	{
		for (auto const &gene: source._genes)
		{
			this->_umis_number += this->_genes[gene.first].merge(gene.second);
		}
	}

	void Cell::merge_umis(const std::string &gene, const s_s_hash_t &merge_targets)
	{
		auto &gene_umis = this->_genes.at(gene);
		for (auto const &target: merge_targets)
		{
			if (target.second == target.first)
				continue;

			gene_umis.merge(target.first, target.second);
			this->_umis_number--;
		}
	}

	const Cell::genes_t &Cell::genes() const
	{
		return this->_genes;
	}

	Cell::s_ul_hash_t Cell::requested_umis_per_gene(bool return_reads) const
	{
		s_ul_hash_t umis_per_gene;
		for (auto const &gene : this->_genes)
		{
			size_t umis_num = gene.second.number_of_requested_umis(this->_query_marks, return_reads);

			if (umis_num == 0)
				continue;

			umis_per_gene.emplace(gene.first, umis_num);
		}

		return umis_per_gene;
	}

	Cell::ss_ul_hash_t Cell::requested_reads_per_umi_per_gene() const
	{
		ss_ul_hash_t reads_per_umi_per_gene;
		for (auto const &gene : this->_genes)
		{
			s_ul_hash_t reads_per_umi(gene.second.requested_reads_per_umi(this->_query_marks));
			if (reads_per_umi.empty())
				continue;

			reads_per_umi_per_gene.emplace(gene.first, reads_per_umi);
		}

		return reads_per_umi_per_gene;
	}

	bool Cell::is_merged() const
	{
		return this->_is_merged;
	}

	bool Cell::is_excluded() const
	{
		return this->_is_excluded;
	}

	const std::string &Cell::barcode() const
	{
		return this->_barcode;
	}

	size_t Cell::umis_number() const
	{
		return this->_umis_number;
	}

	size_t Cell::requested_genes_num() const
	{
		return this->_requested_genes_num;
	}

	size_t Cell::requested_umis_num() const
	{
		return this->_requested_umis_num;
	}

	size_t Cell::size() const
	{
		return this->_genes.size();
	}

	bool Cell::is_real() const
	{
		return !this->_is_excluded && !this->_is_merged && this->size() >= this->_min_genes_to_be_real;
	}

	void Cell::update_requested_size()
	{
		this->_requested_genes_num = 0;
		this->_requested_umis_num = 0;
		for (auto const &gene : this->_genes)
		{
			size_t cur_num = gene.second.number_of_requested_umis(this->_query_marks, false);

			this->_requested_umis_num += cur_num;
			if (cur_num > 0)
			{
				this->_requested_genes_num++;
			}
		}
	}

	const Gene& Cell::at(const std::string &gene) const
	{
		return this->_genes.at(gene);
	}
}