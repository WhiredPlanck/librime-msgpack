#include <algorithm>
#include <iostream>
#include <string>
#include <rime_msgpack_api.h>
#include "msgpack/msgpack.hpp"
#include "rime_ipc.h"
#include "rime_msgpack.h"

void print_status(const StatusType &status) {
  std::cout << "schema: " << status.schemaId.c_str()
            << " / " << status.schemaName.c_str() << std::endl;
  std::cout << "status:";
  if (status.isDisabled) std::cout << " disabled";
  if (status.isComposing) std::cout << " composing";
  if (status.isAsciiMode) std::cout << " ascii";
  if (status.isFullShape) std::cout << " full_shape";
  if (status.isSimplified) std::cout << " simplified";
  std::cout << std::endl;
}

void print_composition(const ContextType::CompositionType &composition) {
  if (composition.preedit.empty()) return;
  std::string preedit = composition.preedit;
  size_t len = composition.length;
  size_t start = composition.selStart;
  size_t end = composition.selEnd;
  size_t cursor = composition.cursorPos;
  for (size_t i = 0; i <= len; ++i) {
    if (start < end) {
      if (i == start) {
        std::cout << '[';
      } else if (i == end) {
        std::cout << ']';
      }
    }
    if (i == cursor) std::cout << '|';
    if (i < len)
      std::cout << preedit[i];
  }
  std::cout << std::endl;
}

void print_menu(const ContextType::MenuType &menu) {
  int num_candidates = menu.candidates.size();
  if (num_candidates == 0) return;
  std::cout << "page: " << (menu.pageNumber + 1)
            << (menu.isLastPage ? '$' : ' ')
            << " (of size " << menu.pageSize << ")" << std::endl;
  for (int i = 0; i < num_candidates; ++i) {
    bool highlighted = i == menu.highlightedCandidateIndex;
    auto candidate = menu.candidates[i];
    std::cout << (i + 1) << ". "
              << (highlighted ? '[' : ' ')
              << candidate.text.c_str()
              << (highlighted ? ']' : ' ');
    if (!candidate.comment.empty())
      std::cout << candidate.comment.c_str();
    std::cout << std::endl;
  }
}

void print_context(const ContextType &context) {
  auto &composition = context.composition;
  if (composition.length > 0) {
    print_composition(composition);
    print_menu(context.menu);
  } else {
    std::cout << "(not composing)" << std::endl;
  }
}

void print(RimeSessionId session_id) {
  RimeApi *rime = rime_get_api();
  RimeMsgpackApi *pack = (RimeMsgpackApi *) rime->find_module("msgpack")->get_api();

  RimeIpcResponce root;

  std::vector<uint8_t> commit_data;
  auto &commit = root.commit;
  pack->commit_msgpack(session_id, &commit_data);
  commit = msgpack::unpack<CommitType>(commit_data);
  if (!commit.text.empty()) {
    std::cout << "commit: " << commit.text.c_str() << std::endl;
  }

  std::vector<uint8_t> status_data;
  pack->status_msgpack(session_id, &status_data);
  root.status = msgpack::unpack<StatusType>(status_data);
  print_status(root.status);

  std::vector<uint8_t> context_data;
  pack->context_msgpack(session_id, &context_data);
  root.context = msgpack::unpack<ContextType>(context_data);
  print_context(root.context);
}

inline static bool is_prefix_of(const std::string& str,
                                const std::string& prefix) {
  return std::mismatch(prefix.begin(),
                       prefix.end(),
                       str.begin()).first == prefix.end();
}

bool execute_special_command(const std::string& line,
                             RimeSessionId session_id) {
  RimeApi* rime = rime_get_api();
  if (line == "print schema list") {
    RimeSchemaList list;
    if (rime->get_schema_list(&list)) {
      std::cout << "schema list:" << std::endl;
      for (size_t i = 0; i < list.size; ++i) {
        std::cout << (i + 1) << ". " << list.list[i].name
                  << " [" << list.list[i].schema_id << "]" << std::endl;
      }
      rime->free_schema_list(&list);
    }
    char current[100] = {0};
    if (rime->get_current_schema(session_id, current, sizeof(current))) {
      std::cout << "current schema: [" << current << "]" << std::endl;
    }
    return true;
  }
  const std::string kSelectSchema = "select schema ";
  if (is_prefix_of(line, kSelectSchema)) {
    auto schema_id = line.substr(kSelectSchema.length());
    if (rime->select_schema(session_id, schema_id.c_str())) {
      std::cout << "selected schema: [" << schema_id << "]" << std::endl;
    }
    return true;
  }
  const std::string kSelectCandidate = "select candidate ";
  if (is_prefix_of(line, kSelectCandidate)) {
    int index = std::stoi(line.substr(kSelectCandidate.length()));
    if (index > 0 &&
        rime->select_candidate_on_current_page(session_id, index - 1)) {
      print(session_id);
    } else {
      std::cerr << "cannot select candidate at index " << index << "."
                << std::endl;
    }
    return true;
  }
  if (line == "print candidate list") {
    RimeCandidateListIterator iterator = {0};
    if (rime->candidate_list_begin(session_id, &iterator)) {
      while (rime->candidate_list_next(&iterator)) {
        std::cout << (iterator.index + 1) << ". " << iterator.candidate.text;
        if (iterator.candidate.comment)
          std::cout << " (" << iterator.candidate.comment << ")";
        std::cout << std::endl;
      }
      rime->candidate_list_end(&iterator);
    } else {
      std::cout << "no candidates." << std::endl;
    }
    return true;
  }
  const std::string kSetOption = "set option ";
  if (is_prefix_of(line, kSetOption)) {
    Bool is_on = True;
    auto iter = line.begin() + kSetOption.length();
    const auto end = line.end();
    if (iter != end && *iter == '!') {
      is_on = False;
      ++iter;
    }
    auto option = std::string(iter, end);
    rime->set_option(session_id, option.c_str(), is_on);
    std::cout << option << " set " << (is_on ? "on" : "off") << std::endl;
    return true;
  }
  return false;
}

void on_message(void* context_object,
                RimeSessionId session_id,
                const char* message_type,
                const char* message_value) {
  std::cout << "message: [" << session_id << "] [" << message_type << "] "
            << message_value << std::endl;
}

int main(int argc, char *argv[]) {
  RimeApi* rime = rime_get_api();

  RIME_STRUCT(RimeTraits, traits);
  traits.app_name = "rime.console";
  rime->setup(&traits);

  rime->set_notification_handler(&on_message, NULL);

  std::cerr << "initializing..." << std::endl;
  rime->initialize(NULL);
  Bool full_check = True;
  if (rime->start_maintenance(full_check))
    rime->join_maintenance_thread();
  std::cerr << "ready." << std::endl;

  RimeSessionId session_id = rime->create_session();
  if (!session_id) {
    std::cerr << "Error creating rime session." << std::endl;
    return 1;
  }

  std::string line;
  while (std::getline(std::cin, line)) {
    if (line == "exit")
      break;
    if (execute_special_command(line, session_id))
      continue;
    if (rime->simulate_key_sequence(session_id, line.c_str())) {
      print(session_id);
    } else {
      std::cerr << "Error processing key sequence: " << line << std::endl;
    }
  }

  rime->destroy_session(session_id);
  rime->finalize();
  return 0;
}
