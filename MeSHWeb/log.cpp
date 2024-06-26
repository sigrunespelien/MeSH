#include "log.h"

#include <Wt/WDate.h>

#include "application.h"


MeshLog::MeshLog(const MeSHApplication* mesh_application)
: m_mesh_application(mesh_application)
{
}

void MeshLog::LogSearch(const std::string& search_string)
{
    //Update search text statistics
    int count = 0;
    Json::Object text_search_result;
	auto es_util = m_mesh_application->GetElasticSearchUtil();
    if (es_util->getDocument("text_statistics", search_string.c_str(), text_search_result))
    {
        const Json::Value source_value = text_search_result.getValue("_source");
        const Json::Object source_object = source_value.getObject();
        if (source_object.member("count"))
        {
            const Json::Value count_value = source_object.getValue("count");
            count = count_value.getInt();
        }
    }
    
    Json::Object textstat_json;
    textstat_json.addMemberByKey("count", ++count);
    es_util->upsert("text_statistics", search_string, textstat_json);


    //Update search day statistics
    Wt::WDate today = Wt::WDate::currentServerDate();
    const std::string today_string = today.toString("yyyy-MM-dd").toUTF8();
    count = 0;
    Json::Object day_search_result;
    if (es_util->getDocument("day_statistics", today_string.c_str(), day_search_result))
    {
        const Json::Value source_value = day_search_result.getValue("_source");
        const Json::Object source_object = source_value.getObject();
        if (source_object.member("count"))
        {
            const Json::Value count_value = source_object.getValue("count");
            count = count_value.getInt();
        }
    }
    
    Json::Object daystat_json;
    daystat_json.addMemberByKey("count", ++count);
    es_util->upsert("day_statistics", today_string, daystat_json);
}
