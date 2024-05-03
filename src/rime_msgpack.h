#pragma once

#include <string>
#include <vector>

typedef struct commit_t {
    std::string text;

    template<typename T>
    void pack(T &pack) {
        pack(text);
    }
} CommitType;

typedef struct candidate_t {
    std::string text;
    std::string comment;
    std::string label;

    template<typename T>
    void pack(T &pack) {
        pack(text, comment, label);
    }
} CandidateType;


typedef struct context_t {

    typedef struct composition_t {
        int32_t length;
        int32_t cursorPos;
        int32_t selStart;
        int32_t selEnd;
        std::string preedit;
        std::string commitTextPreview;

        template<typename T>
        void pack(T &pack) {
            pack(length, cursorPos, selStart, selEnd, preedit, commitTextPreview);
        }
    } CompositionType;

    typedef struct menu_t {
        int32_t pageSize;
        int32_t pageNumber;
        bool isLastPage;
        int32_t highlightedCandidateIndex;
        std::vector<CandidateType> candidates;
        std::string selectKeys;
        std::vector<std::string> selectLabels;

        template<typename T>
        void pack(T &pack) {
            pack(pageSize, pageNumber, isLastPage, highlightedCandidateIndex, candidates, selectKeys, selectLabels);
        }
    } MenuType;

    CompositionType composition;
    MenuType menu;
    std::string input;
    int32_t caretPos;

    template<typename T>
    void pack(T &pack) {
        pack(composition, menu, input, caretPos);
    }
} ContextType;

typedef struct status_t {
    std::string schemaId;
    std::string schemaName;
    bool isDisabled;
    bool isComposing;
    bool isAsciiMode;
    bool isFullShape;
    bool isSimplified;
    bool isTraditional;
    bool isAsciiPunct;

    template<typename T>
    void pack(T &pack) {
        pack(schemaId, schemaName, isDisabled, isComposing, isAsciiMode, isFullShape, isSimplified, isTraditional, isAsciiPunct);
    }
} StatusType;
