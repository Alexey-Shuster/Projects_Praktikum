#include "json_reader.h"
#include "transport_router.h"
#include <iostream>

/*
     * Примерная структура программы:
     *
     * Считать JSON из stdin
     * Построить на его основе JSON базу данных транспортного справочника
     * Выполнить запросы к справочнику, находящиеся в массива "stat_requests", построив JSON-массив
     * с ответами Вывести в stdout ответы в виде JSON
     */

using namespace std;

int main()
{
    TransportCatalogue catalogue;
    renderer::MapRenderer renderer(catalogue);
    transport_router::TransportRouter router(catalogue);
    json_reader::JSONReader reader(json::Load(std::cin), catalogue, renderer, std::move(router));

    reader.ApplyCommandsBase();
    // reader.PrintRouteRequestStat();
    json::Print(reader.PrintStat(), std::cout);

    return 0;
}
