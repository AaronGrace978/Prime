"""Capture Prime Studio UI screenshots for docs/PR."""
import asyncio
from pathlib import Path
from playwright.async_api import async_playwright

OUT = Path("/opt/cursor/artifacts")
OUT.mkdir(parents=True, exist_ok=True)


async def main():
    async with async_playwright() as p:
        browser = await p.chromium.launch()
        page = await browser.new_page(viewport={"width": 1400, "height": 900})
        await page.goto("http://127.0.0.1:8787/", wait_until="networkidle")
        await page.wait_for_timeout(3000)  # auto-generate on load
        await page.screenshot(path=str(OUT / "prime-studio-panel.png"), full_page=True)

        # Generate lava preset
        await page.fill("#prompt", "molten lava flow with glowing orange cracks and emissive heat veins")
        await page.click("#generate")
        await page.wait_for_timeout(2500)
        await page.screenshot(path=str(OUT / "prime-studio-lava.png"), full_page=True)

        # Generate ice
        await page.fill("#prompt", "frozen ice crystal surface pale blue with sharp cell patterns")
        await page.click("#generate")
        await page.wait_for_timeout(2500)
        await page.screenshot(path=str(OUT / "prime-studio-ice.png"), full_page=True)

        await browser.close()
        print("Screenshots saved to", OUT)


asyncio.run(main())
