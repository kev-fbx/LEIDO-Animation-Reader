import QtQuick

// Small glossy 16x16 icons in the Windows 7 manner, painted so no image assets are needed.
// kind: "play" | "record" | "stop" | "export" | "folder" | "file" | "take" | "check"
Canvas {
    id: canvas
    property string kind: ""
    property bool dimmed: false

    width: 16
    height: 16
    opacity: dimmed ? 0.45 : 1
    onKindChanged: requestPaint()

    function glossyCircle(ctx, cx, cy, r, light, dark, edge) {
        const g = ctx.createLinearGradient(0, cy - r, 0, cy + r)
        g.addColorStop(0, light)
        g.addColorStop(1, dark)
        ctx.beginPath()
        ctx.arc(cx, cy, r, 0, Math.PI * 2)
        ctx.fillStyle = g
        ctx.fill()
        ctx.lineWidth = 1
        ctx.strokeStyle = edge
        ctx.stroke()
        // gloss highlight on the upper half
        const h = ctx.createLinearGradient(0, cy - r, 0, cy)
        h.addColorStop(0, "rgba(255,255,255,0.85)")
        h.addColorStop(1, "rgba(255,255,255,0.05)")
        ctx.beginPath()
        ctx.ellipse(cx - r * 0.7, cy - r * 0.92, r * 1.4, r * 0.95)
        ctx.fillStyle = h
        ctx.fill()
    }

    function page(ctx, x, y, w, h) {
        ctx.beginPath()
        ctx.moveTo(x + 0.5, y + 0.5)
        ctx.lineTo(x + w - 4.5, y + 0.5)
        ctx.lineTo(x + w - 0.5, y + 4.5)
        ctx.lineTo(x + w - 0.5, y + h - 0.5)
        ctx.lineTo(x + 0.5, y + h - 0.5)
        ctx.closePath()
        const g = ctx.createLinearGradient(x, y, x + w, y + h)
        g.addColorStop(0, "#FFFFFF")
        g.addColorStop(1, "#E3E8EF")
        ctx.fillStyle = g
        ctx.fill()
        ctx.strokeStyle = "#8C9AAE"
        ctx.lineWidth = 1
        ctx.stroke()
        ctx.beginPath() // folded corner
        ctx.moveTo(x + w - 4.5, y + 0.5)
        ctx.lineTo(x + w - 4.5, y + 4.5)
        ctx.lineTo(x + w - 0.5, y + 4.5)
        ctx.stroke()
    }

    onPaint: {
        const ctx = getContext("2d")
        ctx.reset()
        switch (kind) {
        case "play": {
            const g = ctx.createLinearGradient(0, 2, 0, 14)
            g.addColorStop(0, "#9BE59B")
            g.addColorStop(1, "#1E8F2E")
            ctx.beginPath()
            ctx.moveTo(4.5, 2.5)
            ctx.lineTo(13.5, 8)
            ctx.lineTo(4.5, 13.5)
            ctx.closePath()
            ctx.fillStyle = g
            ctx.fill()
            ctx.strokeStyle = "#1A6B26"
            ctx.lineWidth = 1
            ctx.stroke()
            break
        }
        case "record":
            glossyCircle(ctx, 8, 8, 5.5, "#FF8A7A", "#C40F0F", "#8E0B0B")
            break
        case "stop": {
            const g = ctx.createLinearGradient(0, 3, 0, 13)
            g.addColorStop(0, "#8EA3BE")
            g.addColorStop(1, "#3B5577")
            ctx.fillStyle = g
            ctx.fillRect(3.5, 3.5, 9, 9)
            ctx.strokeStyle = "#2B4060"
            ctx.lineWidth = 1
            ctx.strokeRect(3.5, 3.5, 9, 9)
            break
        }
        case "file":
        case "take":
            page(ctx, 2, 1, 12, 14)
            if (kind === "take") { // a film strip on the page: it is a recording
                ctx.fillStyle = "#3E6FB3"
                ctx.fillRect(4, 6, 8, 5)
                ctx.fillStyle = "#FFFFFF"
                for (let i = 0; i < 4; ++i) {
                    ctx.fillRect(4.5 + i * 2, 6.5, 1, 1)
                    ctx.fillRect(4.5 + i * 2, 9.5, 1, 1)
                }
            }
            break
        case "export": {
            page(ctx, 1, 1, 11, 14)
            const g = ctx.createLinearGradient(0, 6, 0, 14)
            g.addColorStop(0, "#7FC6F5")
            g.addColorStop(1, "#1C6CC1")
            ctx.beginPath() // arrow pointing right
            ctx.moveTo(6.5, 8.5)
            ctx.lineTo(10.5, 8.5)
            ctx.lineTo(10.5, 6.5)
            ctx.lineTo(15.5, 10.5)
            ctx.lineTo(10.5, 14.5)
            ctx.lineTo(10.5, 12.5)
            ctx.lineTo(6.5, 12.5)
            ctx.closePath()
            ctx.fillStyle = g
            ctx.fill()
            ctx.strokeStyle = "#174F8F"
            ctx.stroke()
            break
        }
        case "folder": {
            ctx.fillStyle = "#E3B341"
            ctx.fillRect(1.5, 3.5, 6, 2)
            const g = ctx.createLinearGradient(0, 5, 0, 14)
            g.addColorStop(0, "#FCE7A6")
            g.addColorStop(1, "#E6B13E")
            ctx.fillStyle = g
            ctx.fillRect(1.5, 5.5, 13, 8)
            ctx.strokeStyle = "#B8862B"
            ctx.lineWidth = 1
            ctx.strokeRect(1.5, 5.5, 13, 8)
            ctx.strokeRect(1.5, 3.5, 6, 2)
            break
        }
        case "check": {
            ctx.beginPath()
            ctx.moveTo(3, 8.5)
            ctx.lineTo(6.5, 12)
            ctx.lineTo(13, 4)
            ctx.lineWidth = 2.2
            ctx.strokeStyle = "#2E8B2E"
            ctx.stroke()
            break
        }
        }
    }
}
