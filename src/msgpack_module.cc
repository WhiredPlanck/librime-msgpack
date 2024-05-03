#include <rime/component.h>
#include <rime/composition.h>
#include <rime/context.h>
#include <rime/menu.h>
#include <rime/schema.h>
#include <rime/service.h>
#include <rime_api.h>
#include "rime_msgpack_api.h"
#include "rime_msgpack.h"
#include "msgpack/msgpack.hpp"

using namespace rime;

void rime_commit_msgpack(RimeSessionId session_id, RIME_MSGPACK_TYPE *commit_data) {
    an<Session> session(Service::instance().GetSession(session_id));
    if (!session)
        return;
    const string& commit_text(session->commit_text());
    if (!commit_text.empty()) {
        auto *data = (std::vector<uint8_t> *) commit_data;
        CommitType commit {commit_text};
        *data = msgpack::pack(commit);
        session->ResetCommitText();
    }
};

void rime_context_msgpack(RimeSessionId session_id, RIME_MSGPACK_TYPE *context_data) {
    an<Session> session = Service::instance().GetSession(session_id);
    if (!session)
        return;
    Context *ctx = session->context();
    if (!ctx)
        return;
    auto *data = (std::vector<uint8_t> *) context_data;
    ContextType context;
    context.input = ctx->input();
    context.caretPos = ctx->caret_pos();
    if (ctx->IsComposing()) {
        Preedit preedit = ctx->GetPreedit();
        ContextType::CompositionType composition {
            static_cast<int32_t>(preedit.text.length()),
            static_cast<int32_t>(preedit.caret_pos),
            static_cast<int32_t>(preedit.sel_start),
            static_cast<int32_t>(preedit.sel_end),
            preedit.text
        };
        string commit_text = ctx->GetCommitText();
        if (!commit_text.empty()) {
            composition.commitTextPreview = commit_text;
        }
        context.composition = composition;
    }
    if (ctx->HasMenu()) {
        Segment &seg = ctx->composition().back();
        Schema *schema = session->schema();
        int page_size = schema ? schema->page_size() : 5;
        int selected_index = seg.selected_index;
        int page_number = selected_index / page_size;
        the<Page> page(seg.menu->CreatePage(page_size, page_number));
        if (page) {
            ContextType::MenuType menu {
                page_size,
                page_number,
                page->is_last_page,
                selected_index % page_size
            };
            vector<string> labels;
            if (schema) {
                const string& select_keys = schema->select_keys();
                if (!select_keys.empty()) {
                    menu.selectKeys = select_keys;
                }
                Config* config = schema->config();
                auto src_labels = config->GetList("menu/alternative_select_labels");
                if (src_labels && (size_t)page_size <= src_labels->size()) {
                    auto &dest_labels = menu.selectLabels;
                    dest_labels.reserve(page_size);
                    for (size_t i = 0; i < (size_t)page_size; ++i) {
                        if (an<ConfigValue> value = src_labels->GetValueAt(i)) {
                            dest_labels.emplace_back(value->str());
                            labels.emplace_back(value->str());
                        }
                    }
                } else if (!select_keys.empty()) {
                    for (const char key : select_keys) {
                        labels.push_back(string(1, key));
                        if (labels.size() >= page_size)
                            break;
                    }
                }
            }
            int num_candidates = page->candidates.size();
            auto &dest_candidates = menu.candidates;
            dest_candidates.reserve(num_candidates);
            int index = 0;
            for (const an<Candidate> &src : page->candidates) {
                CandidateType candidate {src->text()};
                string comment = src->comment();
                if (!comment.empty()) {
                    candidate.comment = comment;
                }
                string label = index < labels.size()
                    ? labels[index]
                    : std::to_string(index + 1);
                candidate.label = label;
                dest_candidates.emplace_back(candidate);
            }
            context.menu = menu;
        }
    }
    *data = msgpack::pack(context);
};

void rime_status_msgpack(RimeSessionId session_id, RIME_MSGPACK_TYPE *status_data) {
    an<Session> session(Service::instance().GetSession(session_id));
    if (!session)
        return;
    Schema *schema = session->schema();
    Context *ctx = session->context();
    if (!schema || !ctx)
        return;
    auto *data = (std::vector<uint8_t> *) status_data;
    StatusType status {
        schema->schema_id(),
        schema->schema_name(),
        Service::instance().disabled(),
        ctx->IsComposing(),
        ctx->get_option("ascii_mode"),
        ctx->get_option("full_shape"),
        ctx->get_option("simplification"),
        ctx->get_option("traditional"),
        ctx->get_option("ascii_punct")
    };
    *data = msgpack::pack(status);
};

static void rime_msgpack_initialize() {}

static void rime_msgpack_finalize() {}

static RimeCustomApi* rime_msgpack_get_api() {
    static RimeMsgpackApi s_api = {0};
    if (!s_api.data_size) {
        RIME_STRUCT_INIT(RimeMsgpackApi, s_api);
        s_api.commit_msgpack = &rime_commit_msgpack;
        s_api.context_msgpack = &rime_context_msgpack;
        s_api.status_msgpack = &rime_status_msgpack;
    }
    return (RimeCustomApi*) &s_api;
}

RIME_REGISTER_CUSTOM_MODULE(msgpack) {
    module->get_api = &rime_msgpack_get_api;
}
