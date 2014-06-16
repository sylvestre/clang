//===--- PlistDiagnostics.cpp - Plist Diagnostics for Paths -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the PlistDiagnostics object.
//
//===----------------------------------------------------------------------===//

#include "clang/StaticAnalyzer/Core/AnalyzerOptions.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/PlistSupport.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/Version.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/StaticAnalyzer/Core/BugReporter/PathDiagnostic.h"
#include "clang/StaticAnalyzer/Core/PathDiagnosticConsumers.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/Support/Casting.h"
#include "clang/Index/USRGeneration.h"
#include "llvm/Support/MD5.h"

using namespace clang;
using namespace ento;
using namespace markup;

namespace {
  class PlistDiagnostics : public PathDiagnosticConsumer {
    const std::string OutputFile;
    const LangOptions &LangOpts;
    const bool SupportsCrossFileDiagnostics;
  public:
    PlistDiagnostics(AnalyzerOptions &AnalyzerOpts,
                     const std::string& prefix,
                     const LangOptions &LangOpts,
                     bool supportsMultipleFiles);

    virtual ~PlistDiagnostics() {}

    void FlushDiagnosticsImpl(std::vector<const PathDiagnostic *> &Diags,
                              FilesMade *filesMade) override;

    virtual StringRef getName() const override {
      return "PlistDiagnostics";
    }

    PathGenerationScheme getGenerationScheme() const override {
      return Extensive;
    }
    bool supportsLogicalOpControlFlow() const override { return true; }
    bool supportsCrossFileDiagnostics() const override {
      return SupportsCrossFileDiagnostics;
    }

    SmallString<32> hashIssue(const PathDiagnostic &D,
                              const SourceManager &sourceManager );


  };
} // end anonymous namespace

PlistDiagnostics::PlistDiagnostics(AnalyzerOptions &AnalyzerOpts,
                                   const std::string& output,
                                   const LangOptions &LO,
                                   bool supportsMultipleFiles)
  : OutputFile(output),
    LangOpts(LO),
    SupportsCrossFileDiagnostics(supportsMultipleFiles) {}

void ento::createPlistDiagnosticConsumer(AnalyzerOptions &AnalyzerOpts,
                                         PathDiagnosticConsumers &C,
                                         const std::string& s,
                                         const Preprocessor &PP) {
  C.push_back(new PlistDiagnostics(AnalyzerOpts, s,
                                   PP.getLangOpts(), false));
}

void ento::createPlistMultiFileDiagnosticConsumer(AnalyzerOptions &AnalyzerOpts,
                                                  PathDiagnosticConsumers &C,
                                                  const std::string &s,
                                                  const Preprocessor &PP) {
  C.push_back(new PlistDiagnostics(AnalyzerOpts, s,
                                   PP.getLangOpts(), true));
}

SmallString<32>
PlistDiagnostics::hashIssue(const PathDiagnostic &D,
                            const SourceManager &sourceManager ) {

  StringRef fileName = sourceManager.getFilename(
    static_cast<SourceLocation>(D.getLocation().asLocation()));

  SmallString<1024> USR;
  index::generateUSRForDecl(D.getDeclWithIssue(), USR);

  StringRef description = D.getVerboseDescription();
  StringRef type = D.getBugType();
  StringRef category = D.getCategory();


  llvm::MD5 issueId;
  issueId.update( fileName );
  issueId.update( USR );
  issueId.update( description );
  issueId.update( type );
  issueId.update( category );

  llvm::MD5::MD5Result MD5Res;
  issueId.final(MD5Res);
  SmallString<32> Res;
  llvm::MD5::stringifyResult(MD5Res, Res);

  return Res;
}

static void ReportControlFlow(raw_ostream &o,
                              const PathDiagnosticControlFlowPiece& P,
                              const FIDMap& FM,
                              const SourceManager &SM,
                              const LangOptions &LangOpts,
                              unsigned indent) {

  Indent(o, indent) << "<dict>\n";
  ++indent;

  Indent(o, indent) << "<key>kind</key><string>control</string>\n";

  // Emit edges.
  Indent(o, indent) << "<key>edges</key>\n";
  ++indent;
  Indent(o, indent) << "<array>\n";
  ++indent;
  for (PathDiagnosticControlFlowPiece::const_iterator I=P.begin(), E=P.end();
       I!=E; ++I) {
    Indent(o, indent) << "<dict>\n";
    ++indent;

    // Make the ranges of the start and end point self-consistent with adjacent edges
    // by forcing to use only the beginning of the range.  This simplifies the layout
    // logic for clients.
    Indent(o, indent) << "<key>start</key>\n";
    SourceLocation StartEdge = I->getStart().asRange().getBegin();
    EmitRange(o, SM, LangOpts, CharSourceRange::getTokenRange(StartEdge), FM,
              indent + 1);

    Indent(o, indent) << "<key>end</key>\n";
    SourceLocation EndEdge = I->getEnd().asRange().getBegin();
    EmitRange(o, SM, LangOpts, CharSourceRange::getTokenRange(EndEdge), FM,
              indent + 1);

    --indent;
    Indent(o, indent) << "</dict>\n";
  }
  --indent;
  Indent(o, indent) << "</array>\n";
  --indent;

  // Output any helper text.
  const std::string& s = P.getString();
  if (!s.empty()) {
    Indent(o, indent) << "<key>alternate</key>";
    EmitString(o, s) << '\n';
  }

  --indent;
  Indent(o, indent) << "</dict>\n";
}

static void ReportEvent(raw_ostream &o, const PathDiagnosticPiece& P,
                        const FIDMap& FM,
                        const SourceManager &SM,
                        const LangOptions &LangOpts,
                        unsigned indent,
                        unsigned depth,
                        bool isKeyEvent = false) {

  Indent(o, indent) << "<dict>\n";
  ++indent;

  Indent(o, indent) << "<key>kind</key><string>event</string>\n";

  if (isKeyEvent) {
    Indent(o, indent) << "<key>key_event</key><true/>\n";
  }

  // Output the location.
  FullSourceLoc L = P.getLocation().asLocation();

  Indent(o, indent) << "<key>location</key>\n";
  EmitLocation(o, SM, LangOpts, L, FM, indent);

  // Output the ranges (if any).
  ArrayRef<SourceRange> Ranges = P.getRanges();

  if (!Ranges.empty()) {
    Indent(o, indent) << "<key>ranges</key>\n";
    Indent(o, indent) << "<array>\n";
    ++indent;
    for (ArrayRef<SourceRange>::iterator I = Ranges.begin(), E = Ranges.end();
         I != E; ++I) {
      EmitRange(o, SM, LangOpts, CharSourceRange::getTokenRange(*I), FM,
                indent + 1);
    }
    --indent;
    Indent(o, indent) << "</array>\n";
  }
  
  // Output the call depth.
  Indent(o, indent) << "<key>depth</key>"
                    << "<integer>" << depth << "</integer>\n";

  // Output the text.
  assert(!P.getString().empty());
  Indent(o, indent) << "<key>extended_message</key>\n";
  Indent(o, indent);
  EmitString(o, P.getString()) << '\n';

  // Output the short text.
  // FIXME: Really use a short string.
  Indent(o, indent) << "<key>message</key>\n";
  Indent(o, indent);
  EmitString(o, P.getString()) << '\n';
  
  // Finish up.
  --indent;
  Indent(o, indent); o << "</dict>\n";
}

static void ReportPiece(raw_ostream &o,
                        const PathDiagnosticPiece &P,
                        const FIDMap& FM, const SourceManager &SM,
                        const LangOptions &LangOpts,
                        unsigned indent,
                        unsigned depth,
                        bool includeControlFlow,
                        bool isKeyEvent = false);

static void ReportCall(raw_ostream &o,
                       const PathDiagnosticCallPiece &P,
                       const FIDMap& FM, const SourceManager &SM,
                       const LangOptions &LangOpts,
                       unsigned indent,
                       unsigned depth) {
  
  IntrusiveRefCntPtr<PathDiagnosticEventPiece> callEnter =
    P.getCallEnterEvent();  

  if (callEnter)
    ReportPiece(o, *callEnter, FM, SM, LangOpts, indent, depth, true,
                P.isLastInMainSourceFile());

  IntrusiveRefCntPtr<PathDiagnosticEventPiece> callEnterWithinCaller =
    P.getCallEnterWithinCallerEvent();
  
  ++depth;
  
  if (callEnterWithinCaller)
    ReportPiece(o, *callEnterWithinCaller, FM, SM, LangOpts,
                indent, depth, true);
  
  for (PathPieces::const_iterator I = P.path.begin(), E = P.path.end();I!=E;++I)
    ReportPiece(o, **I, FM, SM, LangOpts, indent, depth, true);

  --depth;
  
  IntrusiveRefCntPtr<PathDiagnosticEventPiece> callExit =
    P.getCallExitEvent();

  if (callExit)
    ReportPiece(o, *callExit, FM, SM, LangOpts, indent, depth, true);
}

static void ReportMacro(raw_ostream &o,
                        const PathDiagnosticMacroPiece& P,
                        const FIDMap& FM, const SourceManager &SM,
                        const LangOptions &LangOpts,
                        unsigned indent,
                        unsigned depth) {

  for (PathPieces::const_iterator I = P.subPieces.begin(), E=P.subPieces.end();
       I!=E; ++I) {
    ReportPiece(o, **I, FM, SM, LangOpts, indent, depth, false);
  }
}

static void ReportDiag(raw_ostream &o, const PathDiagnosticPiece& P,
                       const FIDMap& FM, const SourceManager &SM,
                       const LangOptions &LangOpts) {
  ReportPiece(o, P, FM, SM, LangOpts, 4, 0, true);
}

static void ReportPiece(raw_ostream &o,
                        const PathDiagnosticPiece &P,
                        const FIDMap& FM, const SourceManager &SM,
                        const LangOptions &LangOpts,
                        unsigned indent,
                        unsigned depth,
                        bool includeControlFlow,
                        bool isKeyEvent) {
  switch (P.getKind()) {
    case PathDiagnosticPiece::ControlFlow:
      if (includeControlFlow)
        ReportControlFlow(o, cast<PathDiagnosticControlFlowPiece>(P), FM, SM,
                          LangOpts, indent);
      break;
    case PathDiagnosticPiece::Call:
      ReportCall(o, cast<PathDiagnosticCallPiece>(P), FM, SM, LangOpts,
                 indent, depth);
      break;
    case PathDiagnosticPiece::Event:
      ReportEvent(o, cast<PathDiagnosticSpotPiece>(P), FM, SM, LangOpts,
                  indent, depth, isKeyEvent);
      break;
    case PathDiagnosticPiece::Macro:
      ReportMacro(o, cast<PathDiagnosticMacroPiece>(P), FM, SM, LangOpts,
                  indent, depth);
      break;
  }
}

void PlistDiagnostics::FlushDiagnosticsImpl(
                                    std::vector<const PathDiagnostic *> &Diags,
                                    FilesMade *filesMade) {
  // Build up a set of FIDs that we use by scanning the locations and
  // ranges of the diagnostics.
  FIDMap FM;
  SmallVector<FileID, 10> Fids;
  const SourceManager* SM = nullptr;

  if (!Diags.empty())
    SM = &(*(*Diags.begin())->path.begin())->getLocation().getManager();

  
  for (std::vector<const PathDiagnostic*>::iterator DI = Diags.begin(),
       DE = Diags.end(); DI != DE; ++DI) {

    const PathDiagnostic *D = *DI;

    SmallVector<const PathPieces *, 5> WorkList;
    WorkList.push_back(&D->path);

    while (!WorkList.empty()) {
      const PathPieces &path = *WorkList.pop_back_val();

      for (PathPieces::const_iterator I = path.begin(), E = path.end(); I != E;
           ++I) {
        const PathDiagnosticPiece *piece = I->getPtr();
        AddFID(FM, Fids, *SM, piece->getLocation().asLocation());
        ArrayRef<SourceRange> Ranges = piece->getRanges();
        for (ArrayRef<SourceRange>::iterator I = Ranges.begin(),
                                             E = Ranges.end(); I != E; ++I) {
          AddFID(FM, Fids, *SM, I->getBegin());
          AddFID(FM, Fids, *SM, I->getEnd());
        }

        if (const PathDiagnosticCallPiece *call =
            dyn_cast<PathDiagnosticCallPiece>(piece)) {
          IntrusiveRefCntPtr<PathDiagnosticEventPiece>
            callEnterWithin = call->getCallEnterWithinCallerEvent();
          if (callEnterWithin)
            AddFID(FM, Fids, *SM, callEnterWithin->getLocation().asLocation());

          WorkList.push_back(&call->path);
        }
        else if (const PathDiagnosticMacroPiece *macro =
                 dyn_cast<PathDiagnosticMacroPiece>(piece)) {
          WorkList.push_back(&macro->subPieces);
        }
      }
    }
  }

  // Open the file.
  std::string ErrMsg;
  llvm::raw_fd_ostream o(OutputFile.c_str(), ErrMsg, llvm::sys::fs::F_Text);
  if (!ErrMsg.empty()) {
    llvm::errs() << "warning: could not create file: " << OutputFile << '\n';
    return;
  }

  // Write the plist header.
  o << PlistHeader;

  // Write the root object: a <dict> containing...
  //  - "clang_version", the string representation of clang version
  //  - "files", an <array> mapping from FIDs to file names
  //  - "diagnostics", an <array> containing the path diagnostics
  o << "<dict>\n" <<
       " <key>clang_version</key>\n";
  EmitString(o, getClangFullVersion()) << '\n';
  o << " <key>files</key>\n"
       " <array>\n";

  for (SmallVectorImpl<FileID>::iterator I=Fids.begin(), E=Fids.end();
       I!=E; ++I) {
    o << "  ";
    EmitString(o, SM->getFileEntryForID(*I)->getName()) << '\n';
  }

  o << " </array>\n"
       " <key>diagnostics</key>\n"
       " <array>\n";

  for (std::vector<const PathDiagnostic*>::iterator DI=Diags.begin(),
       DE = Diags.end(); DI!=DE; ++DI) {

    o << "  <dict>\n"
         "   <key>path</key>\n";

    const PathDiagnostic *D = *DI;

    o << "   <array>\n";

    for (PathPieces::const_iterator I = D->path.begin(), E = D->path.end(); 
         I != E; ++I)
      ReportDiag(o, **I, FM, *SM, LangOpts);

    o << "   </array>\n";

    // Output the bug type and bug category.
    o << "   <key>description</key>";
    EmitString(o, D->getShortDescription()) << '\n';
    o << "   <key>category</key>";
    EmitString(o, D->getCategory()) << '\n';
    o << "   <key>type</key>";
    EmitString(o, D->getBugType()) << '\n';
    
    // Output information about the semantic context where
    // the issue occurred.
    if (const Decl *DeclWithIssue = D->getDeclWithIssue()) {
      // FIXME: handle blocks, which have no name.
      if (const NamedDecl *ND = dyn_cast<NamedDecl>(DeclWithIssue)) {
        StringRef declKind;
        switch (ND->getKind()) {
          case Decl::CXXRecord:
            declKind = "C++ class";
            break;
          case Decl::CXXMethod:
            declKind = "C++ method";
            break;
          case Decl::ObjCMethod:
            declKind = "Objective-C method";
            break;
          case Decl::Function:
            declKind = "function";
            break;
          default:
            break;
        }
        if (!declKind.empty()) {
          const std::string &declName = ND->getDeclName().getAsString();
          o << "  <key>issue_context_kind</key>";
          EmitString(o, declKind) << '\n';
          o << "  <key>issue_context</key>";
          EmitString(o, declName) << '\n';
        }


        o << "  <key>issue_hash</key><string>"
          << hashIssue( *D, *SM )
          << "</string>\n";
      }
    }

    // Output the location of the bug.
    o << "  <key>location</key>\n";
    EmitLocation(o, *SM, LangOpts, D->getLocation().asLocation(), FM, 2);

    // Output the diagnostic to the sub-diagnostic client, if any.
    if (!filesMade->empty()) {
      StringRef lastName;
      PDFileEntry::ConsumerFiles *files = filesMade->getFiles(*D);
      if (files) {
        for (PDFileEntry::ConsumerFiles::const_iterator CI = files->begin(),
                CE = files->end(); CI != CE; ++CI) {
          StringRef newName = CI->first;
          if (newName != lastName) {
            if (!lastName.empty()) {
              o << "  </array>\n";
            }
            lastName = newName;
            o <<  "  <key>" << lastName << "_files</key>\n";
            o << "  <array>\n";
          }
          o << "   <string>" << CI->second << "</string>\n";
        }
        o << "  </array>\n";
      }
    }

    // Close up the entry.
    o << "  </dict>\n";
  }

  o << " </array>\n";

  // Finish.
  o << "</dict>\n</plist>";  
}
