import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.InputEvent;
import java.awt.event.KeyEvent;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.awt.image.BufferedImage;
import java.io.IOException;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.BitSet;
import java.util.concurrent.ExecutionException;
import javax.imageio.ImageIO;
import javax.swing.AbstractAction;
import javax.swing.BorderFactory;
import javax.swing.DefaultListCellRenderer;
import javax.swing.Icon;
import javax.swing.JButton;
import javax.swing.JComponent;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.KeyStroke;
import javax.swing.ListSelectionModel;
import javax.swing.SwingConstants;
import javax.swing.SwingUtilities;
import javax.swing.SwingWorker;
import javax.swing.filechooser.FileNameExtensionFilter;

public class BitmapFontEditor
{
	private static final int HEADER_HEIGHT = 10;
	private static final int CHAR_WIDTH = 10;
	private static final int CHAR_HEIGHT = 19;
	private static final int SPACING = 1;
	private static final int PIXEL_SIZE = 25;
	private static final Charset CP437 = Charset.forName("IBM437");

	private final JFrame frame = new JFrame("Bitmap Font Editor");
	private final JList<Integer> characters = new JList<>();
	private final JLabel selection = new JLabel("Choose a character", SwingConstants.CENTER);
	private final JLabel status = new JLabel("Open a font bitmap to begin.");
	private final JButton open = new JButton("Open…");
	private final JButton save = new JButton("Save & Convert");
	private final JButton revert = new JButton("Revert Character");
	private final BitSet changedCharacters = new BitSet();
	private final JPanel pixels;
	private final Path converter;
	private BufferedImage bitmap;
	private BufferedImage savedBitmap;
	private Path bitmapPath;
	private int columns;
	private int selected;
	private boolean dirty;
	private boolean saving;

	public BitmapFontEditor(Path directory)
	{
		converter = directory.resolve("convert.js");
		pixels = new JPanel()
		{
			@Override
			protected void paintComponent(Graphics graphics)
			{
				super.paintComponent(graphics);
				if(bitmap == null)
					return;
				for(int y = 0; y < CHAR_HEIGHT; y++)
					for(int x = 0; x < CHAR_WIDTH; x++)
					{
						graphics.setColor(editorPixelIsOn(x, y) ? Color.BLACK : Color.WHITE);
						graphics.fillRect(x * PIXEL_SIZE, y * PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE);
						graphics.setColor(new Color(190, 190, 190));
						graphics.drawRect(x * PIXEL_SIZE, y * PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE);
					}
			}
		};
		pixels.setPreferredSize(new Dimension(CHAR_WIDTH * PIXEL_SIZE + 1, CHAR_HEIGHT * PIXEL_SIZE + 1));
		MouseAdapter painting = new MouseAdapter()
		{
			private boolean ink;
			private boolean active;

			@Override
			public void mousePressed(MouseEvent event)
			{
				active = false;
				if(!SwingUtilities.isLeftMouseButton(event) || !editorPixelIsEditable(event))
					return;
				active = true;
				ink = !editorPixelIsOn(event.getX() / PIXEL_SIZE, event.getY() / PIXEL_SIZE);
				editorPaintPixel(event, ink);
			}

			@Override
			public void mouseDragged(MouseEvent event)
			{
				if(active && (event.getModifiersEx() & InputEvent.BUTTON1_DOWN_MASK) != 0)
					editorPaintPixel(event, ink);
			}

			@Override
			public void mouseReleased(MouseEvent event) { active = false; }
		};
		pixels.addMouseListener(painting);
		pixels.addMouseMotionListener(painting);

		characters.setLayoutOrientation(JList.HORIZONTAL_WRAP);
		characters.setVisibleRowCount(-1);
		characters.setFixedCellWidth(54);
		characters.setFixedCellHeight(65);
		characters.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
		characters.setCellRenderer(new DefaultListCellRenderer()
		{
			@Override
			public Component getListCellRendererComponent(JList<?> list, Object value, int index,
			                                            boolean isSelected, boolean cellHasFocus)
			{
				super.getListCellRendererComponent(list, String.format("%02X", index), index,
				                                  isSelected, cellHasFocus);
				setHorizontalAlignment(SwingConstants.CENTER);
				setHorizontalTextPosition(SwingConstants.CENTER);
				setVerticalTextPosition(SwingConstants.BOTTOM);
				setIcon(new Icon()
				{
					public int getIconWidth() { return CHAR_WIDTH * 2; }
					public int getIconHeight() { return CHAR_HEIGHT * 2; }

					public void paintIcon(Component component, Graphics graphics, int x, int y)
					{
						int sourceX = index % columns * (CHAR_WIDTH + SPACING);
						int sourceY = HEADER_HEIGHT + index / columns * (CHAR_HEIGHT + SPACING);
						graphics.drawImage(bitmap, x, y, x + getIconWidth(), y + getIconHeight(),
						                   sourceX, sourceY, sourceX + CHAR_WIDTH, sourceY + CHAR_HEIGHT, null);
					}
				});
				return this;
			}
		});
		characters.addListSelectionListener(event -> {
			if(characters.getSelectedIndex() < 0)
				return;
			selected = characters.getSelectedIndex();
			String letter = selected >= 32 && selected < 256 && selected != 127
			                ? " · " + new String(new byte[] {(byte) selected}, CP437) : "";
			selection.setText(String.format("Character %d · 0x%02X%s", selected, selected, letter));
			revert.setEnabled(!saving && changedCharacters.get(selected));
			pixels.repaint();
		});

		JPanel toolbar = new JPanel();
		toolbar.add(open);
		toolbar.add(save);
		toolbar.add(revert);
		open.addActionListener(event -> editorChooseBitmap());
		save.addActionListener(event -> editorSave());
		revert.addActionListener(event -> editorRevertCharacter());
		revert.setToolTipText("Restore this character to its last opened or saved pixels.");
		save.setEnabled(false);
		revert.setEnabled(false);
		JPanel drawing = new JPanel(new BorderLayout(0, 12));
		drawing.setBorder(BorderFactory.createEmptyBorder(8, 16, 8, 8));
		drawing.add(selection, BorderLayout.NORTH);
		JPanel canvas = new JPanel();
		canvas.add(pixels);
		drawing.add(canvas, BorderLayout.CENTER);
		drawing.add(new JLabel("Click to toggle · drag to paint", SwingConstants.CENTER), BorderLayout.SOUTH);
		JScrollPane picker = new JScrollPane(characters);
		picker.setPreferredSize(new Dimension(560, 540));
		JPanel content = new JPanel(new BorderLayout(8, 8));
		content.setBorder(BorderFactory.createEmptyBorder(8, 8, 8, 8));
		content.add(toolbar, BorderLayout.NORTH);
		content.add(picker, BorderLayout.CENTER);
		content.add(drawing, BorderLayout.EAST);
		content.add(status, BorderLayout.SOUTH);
		frame.setContentPane(content);
		frame.setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);
		frame.addWindowListener(new WindowAdapter()
		{
			@Override
			public void windowClosing(WindowEvent event)
			{
				if(!saving && editorConfirmDiscard()) {
					frame.dispose();
					System.exit(0);
                }
			}
		});
		int shortcut = Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx();
		editorBindShortcut("save", KeyEvent.VK_S, shortcut, this::editorSave);
		editorBindShortcut("open", KeyEvent.VK_O, shortcut, this::editorChooseBitmap);
		frame.pack();
		frame.setMinimumSize(frame.getSize());
		frame.setLocationByPlatform(true);
		frame.setVisible(true);
		Path initialBitmap = directory.resolve("font.bmp");
		if(Files.exists(initialBitmap))
			editorOpenBitmap(initialBitmap);
	}

	private void editorBindShortcut(String name, int key, int modifiers, Runnable action)
	{
		frame.getRootPane().getInputMap(JComponent.WHEN_IN_FOCUSED_WINDOW)
		     .put(KeyStroke.getKeyStroke(key, modifiers), name);
		frame.getRootPane().getActionMap().put(name, new AbstractAction()
		{
			@Override
			public void actionPerformed(ActionEvent event) { action.run(); }
		});
	}

	private boolean editorConfirmDiscard()
	{
		return !dirty || JOptionPane.showOptionDialog(frame, "Discard unsaved changes?", "Bitmap Font Editor",
		       JOptionPane.DEFAULT_OPTION, JOptionPane.WARNING_MESSAGE, null,
		       new String[] {"Discard", "Cancel"}, "Cancel") == 0;
	}

	private void editorChooseBitmap()
	{
		if(saving)
			return;
		JFileChooser chooser = new JFileChooser(converter.getParent().toFile());
		chooser.setFileFilter(new FileNameExtensionFilter("Bitmap font (*.bmp)", "bmp"));
		chooser.setAcceptAllFileFilterUsed(false);
		if(chooser.showOpenDialog(frame) == JFileChooser.APPROVE_OPTION)
			editorOpenBitmap(chooser.getSelectedFile().toPath().toAbsolutePath());
	}

	static BufferedImage editorReadBitmap(Path path) throws IOException
	{
		BufferedImage image = ImageIO.read(path.toFile());
		if(image == null || image.getWidth() % (CHAR_WIDTH + SPACING) != 0 ||
		   image.getHeight() <= HEADER_HEIGHT || (image.getHeight() - HEADER_HEIGHT) % (CHAR_HEIGHT + SPACING) != 0)
			throw new IOException("Expected 10 × 19 pixel characters, 1 pixel spacing, and a 10 pixel header.");
		return image;
	}

	private void editorOpenBitmap(Path path)
	{
		try
		{
			BufferedImage image = editorReadBitmap(path);
			if(!editorConfirmDiscard())
				return;
			bitmap = image;
			bitmapPath = path;
			editorCaptureSavedBitmap();
			columns = bitmap.getWidth() / (CHAR_WIDTH + SPACING);
			int count = columns * (bitmap.getHeight() - HEADER_HEIGHT) / (CHAR_HEIGHT + SPACING);
			Integer[] codes = new Integer[count];
			for(int i = 0; i < count; i++)
				codes[i] = i;
			characters.setListData(codes);
			characters.setSelectedIndex(0);
			save.setEnabled(true);
			editorUpdateTitle();
			status.setText(count + " characters · " + path);
		}
		catch(IOException error)
		{
			JOptionPane.showMessageDialog(frame, error.getMessage(), "Cannot open bitmap", JOptionPane.ERROR_MESSAGE);
		}
	}

	private boolean editorPixelIsEditable(MouseEvent event)
	{
		return bitmap != null && !saving && event.getX() >= 0 && event.getY() >= 0 &&
		       event.getX() < CHAR_WIDTH * PIXEL_SIZE && event.getY() < CHAR_HEIGHT * PIXEL_SIZE;
	}

	private boolean editorPixelIsOn(int x, int y)
	{
		return (bitmap.getRGB(selected % columns * (CHAR_WIDTH + SPACING) + x,
		        HEADER_HEIGHT + selected / columns * (CHAR_HEIGHT + SPACING) + y) & 0xFFFFFF) == 0;
	}

	private void editorPaintPixel(MouseEvent event, boolean ink)
	{
		if(!editorPixelIsEditable(event))
			return;
		int x = event.getX() / PIXEL_SIZE;
		int y = event.getY() / PIXEL_SIZE;
		if(editorPixelIsOn(x, y) == ink)
			return;
		bitmap.setRGB(selected % columns * (CHAR_WIDTH + SPACING) + x,
		              HEADER_HEIGHT + selected / columns * (CHAR_HEIGHT + SPACING) + y,
		              ink ? Color.BLACK.getRGB() : Color.WHITE.getRGB());
		editorUpdateChanges();
		pixels.repaint();
		characters.repaint();
	}

	private void editorCaptureSavedBitmap()
	{
		savedBitmap = new BufferedImage(bitmap.getColorModel(), bitmap.copyData(null),
		                               bitmap.isAlphaPremultiplied(), null);
		changedCharacters.clear();
		dirty = false;
		revert.setEnabled(false);
	}

	private void editorUpdateChanges()
	{
		int sourceX = selected % columns * (CHAR_WIDTH + SPACING);
		int sourceY = HEADER_HEIGHT + selected / columns * (CHAR_HEIGHT + SPACING);
		boolean changed = false;
		for(int y = 0; y < CHAR_HEIGHT; y++)
			for(int x = 0; x < CHAR_WIDTH; x++)
				changed |= bitmap.getRGB(sourceX + x, sourceY + y) != savedBitmap.getRGB(sourceX + x, sourceY + y);
		changedCharacters.set(selected, changed);
		dirty = !changedCharacters.isEmpty();
		revert.setEnabled(!saving && changed);
		editorUpdateTitle();
	}

	private void editorRevertCharacter()
	{
		if(bitmap == null || saving || !changedCharacters.get(selected))
			return;
		int sourceX = selected % columns * (CHAR_WIDTH + SPACING);
		int sourceY = HEADER_HEIGHT + selected / columns * (CHAR_HEIGHT + SPACING);
		for(int y = 0; y < CHAR_HEIGHT; y++)
			for(int x = 0; x < CHAR_WIDTH; x++)
				bitmap.setRGB(sourceX + x, sourceY + y, savedBitmap.getRGB(sourceX + x, sourceY + y));
		editorUpdateChanges();
		pixels.repaint();
		characters.repaint();
		status.setText("Reverted character " + selected + ".");
	}

	private void editorUpdateTitle()
	{
		frame.setTitle((dirty ? "* " : "") + bitmapPath.getFileName() + " — Bitmap Font Editor");
	}

	static void editorWriteBitmap(Path path, BufferedImage image, Path converter) throws IOException, InterruptedException
	{
		Path temporary = Files.createTempFile(path.getParent(), ".font-", ".bmp");
		try
		{
			BufferedImage output = new BufferedImage(image.getWidth(), image.getHeight(), BufferedImage.TYPE_3BYTE_BGR);
			Graphics graphics = output.createGraphics();
			graphics.drawImage(image, 0, 0, null);
			graphics.dispose();
			if(!ImageIO.write(output, "bmp", temporary.toFile()))
				throw new IOException("No BMP writer is available.");
			Files.move(temporary, path, StandardCopyOption.REPLACE_EXISTING);
		}
		finally
		{
			Files.deleteIfExists(temporary);
		}
		try
		{
			String node = System.getenv().getOrDefault("npm_node_execpath", "node");
			Process process = new ProcessBuilder(node, converter.toString(), path.toString())
			                  .directory(converter.getParent().toFile()).redirectErrorStream(true).start();
			String output = new String(process.getInputStream().readAllBytes(), StandardCharsets.UTF_8);
			if(process.waitFor() != 0)
				throw new IOException(output);
		}
		catch(IOException error)
		{
			throw new IOException("Bitmap saved, but conversion failed:\n" + error.getMessage(), error);
		}
	}

	private void editorSave()
	{
		if(bitmap == null || saving)
			return;
		saving = true;
		open.setEnabled(false);
		save.setEnabled(false);
		revert.setEnabled(false);
		status.setText("Saving bitmap and converting…");
		new SwingWorker<Void, Void>()
		{
			@Override
			protected Void doInBackground() throws IOException, InterruptedException
			{
				editorWriteBitmap(bitmapPath, bitmap, converter);
				return null;
			}

			@Override
			protected void done()
			{
				try
				{
					get();
					editorCaptureSavedBitmap();
					editorUpdateTitle();
					status.setText("Saved " + bitmapPath.getFileName() + " and regenerated the font data.");
				}
				catch(InterruptedException error)
				{
					Thread.currentThread().interrupt();
					status.setText("Save interrupted.");
				}
				catch(ExecutionException error)
				{
					status.setText("Save failed; see error details.");
					JOptionPane.showMessageDialog(frame, error.getCause().getMessage(), "Save failed", JOptionPane.ERROR_MESSAGE);
				}
				finally
				{
					saving = false;
					open.setEnabled(true);
					save.setEnabled(true);
					revert.setEnabled(changedCharacters.get(selected));
				}
			}
		}.execute();
	}

	public static void main(String[] arguments)
	{
		SwingUtilities.invokeLater(() -> new BitmapFontEditor(Path.of("").toAbsolutePath()));
	}
}
