#pragma once

#include <DB/Interpreters/Context.h>
#include <DB/Dictionaries/MysqlBlockInputStream.h>
#include <DB/Dictionaries/IDictionarySource.h>
#include <DB/Dictionaries/config_ptr_t.h>
#include <statdaemons/ext/range.hpp>
#include <mysqlxx/Pool.h>
#include <Poco/Util/LayeredConfiguration.h>

namespace DB
{

class MysqlDictionarySource final : public IDictionarySource
{
	static const auto max_block_size = 8192;

public:
	MysqlDictionarySource(Poco::Util::AbstractConfiguration & config, const std::string & config_prefix,
		Block & sample_block, const Context & context)
		: layered_config_ptr{getLayeredConfig(config)},
		  pool{*layered_config_ptr, config_prefix},
		  sample_block{sample_block}, context(context),
		  table{config.getString(config_prefix + "table")},
		  load_all_query{composeLoadAllQuery(sample_block, table)}
	{}

private:
	BlockInputStreamPtr loadAll() override
	{
		return new MysqlBlockInputStream{pool.Get()->query(load_all_query), sample_block, max_block_size};
	}

	BlockInputStreamPtr loadId(const std::uint64_t id) override
	{
		throw Exception{
			"Method unsupported",
			ErrorCodes::NOT_IMPLEMENTED
		};
	}

	BlockInputStreamPtr loadIds(const std::vector<std::uint64_t> ids) override
	{
		throw Exception{
			"Method unsupported",
			ErrorCodes::NOT_IMPLEMENTED
		};
	}

	static config_ptr_t<Poco::Util::LayeredConfiguration> getLayeredConfig(Poco::Util::AbstractConfiguration & config)
	{
		config_ptr_t<Poco::Util::LayeredConfiguration> layered_config{new Poco::Util::LayeredConfiguration};
		layered_config->add(&config);
		return layered_config;
	}

	static std::string composeLoadAllQuery(const Block & block, const std::string & table)
	{
		std::string query{"SELECT "};

		auto first = true;
		for (const auto idx : ext::range(0, block.columns()))
		{
			if (!first)
				query += ", ";

			query += block.getByPosition(idx).name;
			first = false;
		}

		query += " FROM " + table + ';';

		return query;
	}

	const config_ptr_t<Poco::Util::LayeredConfiguration> layered_config_ptr;
	mysqlxx::Pool pool;
	Block sample_block;
	const Context & context;
	const std::string table;
	const std::string load_all_query;
};

}
