rm -rf out
suds compile -c ./sld57-matter-landing-page/_docleaf-sld57-matter-landing-page.yml -o ./out
suds compile -c _docleaf-sld245-matter-bridge.yml -o ./out
suds compile -c _docleaf-sld246-matter-faq.yml -o ./out
suds compile -c _docleaf-sld247-matter-overview.yml -o ./out
::suds compile -c _docleaf-sld248-matter-overview-guides.yml -o ./out
suds compile -c _docleaf-sld249-matter-prerequisites.yml -o ./out
suds compile -c _docleaf-sld250-matter-references.yml -o ./out
suds compile -c _docleaf-sld251-matter-thread.yml -o ./out
suds compile -c _docleaf-sld252-matter-vscode.yml -o ./out
suds compile -c _docleaf-sld253-matter-wifi.yml -o ./out

