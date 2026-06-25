#include "router.h"

Router &Router::Get(const std::string &pattern, Handler handler) {
    routes_["GET"].push_back({std::regex(pattern), std::move(handler)});
    return *this;
}

Router &Router::Post(const std::string &pattern, Handler handler) {
    routes_["POST"].push_back({std::regex(pattern), std::move(handler)});
    return *this;
}

HttpResponse Router::Dispatch(const HttpRequest &req) const {
    auto it = routes_.find(req.method);
    if (it == routes_.end()) return HttpResponse::NotFound();

    for (const auto &route : it->second) {
        if (std::regex_match(req.path, route.pattern)) {
            return route.handler(req);
        }
    }
    return HttpResponse::NotFound();
}
