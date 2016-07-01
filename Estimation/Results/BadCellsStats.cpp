#include "BadCellsStats.h"

#include "Estimation/Stats.h"
#include "Tools/Logs.h"
#include <Rcpp.h>

namespace Estimation
{
	namespace Results
	{
		BadCellsStats::BadCellsStats(const ss_cnt_t &genes_reads,
		                             const ss_cnt_t &genes_umis, const std::vector<std::string> &excluded_cells)
				: genes_reads(genes_reads)
				, genes_umis(genes_umis)
				, excluded_cells(excluded_cells)
		{}

#ifdef R_LIBS
		void BadCellsStats::save_rds(const Stats &stats, const std::string &filename) const
		{
			using namespace Rcpp;

			L_TRACE << "writing R data to " << filename;
			RInside *R = RInside::instancePtr();
			if (R == nullptr)
			{
				R = new RInside(0, 0);
			}

			(*R)["d"] = List::create(
					Named("genes_reads") = wrap(this->genes_reads),
		            Named("genes_umis") = wrap(this->genes_umis),
		            Named("cell_exone_reads_per_chr") = wrap(stats.get_raw_cell_stat(Stats::EXONE_READS_PER_CHR_PER_CELL)),
		            Named("cell_nonexone_reads_per_chr") = wrap(stats.get_raw_cell_stat(Stats::NON_EXONE_READS_PER_CHR_PER_CELL)),
		            Named("reads_per_umig") = wrap(stats.get_raw_cell_stat(Stats::READS_PER_UMIG_PER_CELL)),
		            Named("reads_per_umi") = wrap(stats.get_raw_cell_stat(Stats::READS_PER_UMI_PER_CELL)),

		            Named("merges_count") = wrap(stats.get_raw_stat(Stats::MERGES_COUNT)),

		            Named("merge_edit_distance") = wrap(stats.get_raw_str_stat(Stats::MERGE_EDIT_DISTANCE_BY_CELL)),
		            Named("merge_intersect_size") = wrap(stats.get_raw_str_stat(Stats::MERGE_INTERSECT_SIZE_BY_CELL)),
		            Named("merge_real_intersect_size") = wrap(stats.get_raw_str_stat(Stats::MERGE_REAL_INTERSECT_SIZE_BY_CELL)),
		            Named("merge_rejections") = wrap(stats.get_raw_str_stat(Stats::MERGE_REJECTION_BY_CELL)),
		            Named("excluded_cells") = wrap(excluded_cells)
			);

			R->parseEvalQ("saveRDS(d, '" + filename + "')");
			L_TRACE << "Done";
		}
#endif
	}
}
