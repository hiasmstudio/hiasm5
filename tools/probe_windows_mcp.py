"""Connect to the installed Windows-MCP over stdio; never send shell commands."""
import argparse
import asyncio
import base64
import json
from pathlib import Path

from fastmcp import Client


async def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--tool')
    parser.add_argument('--arguments', default='{}')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    exe = Path.home() / '.codex/mcp/windows-mcp-venv/Scripts/windows-mcp.exe'
    config = {'mcpServers': {'windows-mcp': {
        'command': str(exe),
        'args': ['serve', '--tools', 'Screenshot,Snapshot,App,Click,Type,Shortcut,Scroll,Drag,Move,Wait,DisplayInventory,ControlStatus'],
        'env': {'ANONYMIZED_TELEMETRY': 'false', 'PYTHONIOENCODING': 'utf-8'},
    }}}
    async with Client(config, timeout=45) as client:
        if not args.tool:
            print(json.dumps([{'name': t.name, 'schema': t.input_schema}
                              for t in await client.list_tools()], ensure_ascii=False))
            return
        result = await client.call_tool(args.tool, json.loads(args.arguments))
        for content in result.content:
            if content.type == 'text':
                print(content.text)
            elif content.type == 'image' and args.output:
                args.output.parent.mkdir(parents=True, exist_ok=True)
                args.output.write_bytes(base64.b64decode(content.data))
                print(f'Screenshot saved: {args.output.resolve()}')


if __name__ == '__main__':
    asyncio.run(main())
