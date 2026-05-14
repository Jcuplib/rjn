# Raijin Coupler

Raijin Coupler is a C++ implementation of the JcupLT coupling library.

## Repository layout

- src: source code
- docs/manual: LaTeX source of the manual
- docs/manual/figs: figures used by the manual
- docs/api: Doxygen configuration for API documentation
- test: test programs and execution directories

## Build the manual

Run:

    cd docs/manual
    make

This generates docs/manual/Raijin.pdf.

The generated PDF is not tracked by git.

## Build the API documentation

Run:

    cd docs/api
    make

This generates docs/api/html/index.html.

Generated HTML files are not tracked by git.

## Dependencies

For the manual:

- TeX Live
- latexmk
- dvipdfmx
- BibTeX

For API documentation:

- Doxygen
- Graphviz

On Fedora:

    sudo dnf install -y texlive-scheme-full latexmk doxygen graphviz

## License

This software is distributed under the BSD 3-Clause License.

Raijin Coupler is derived from the Jcup/JcupLT coupling library.
The original Jcup copyright notice is retained in the LICENSE file.
